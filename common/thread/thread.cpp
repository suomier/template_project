#include "thread.h"

#include <chrono>
#include <iomanip>
#include <sstream>

// 简化版 SocketServer 实现
class SocketServer
{
public:
    virtual ~SocketServer() = default;
    virtual void WakeUp() {}
};

// 创建线程
std::unique_ptr<Thread> Thread::Create()
{
    return std::unique_ptr<Thread>(new Thread(nullptr));
}

// 创建带 SocketServer 的线程
std::unique_ptr<Thread> Thread::CreateWithSocketServer()
{
    return std::unique_ptr<Thread>(new Thread(new SocketServer()));
}

// 构造函数（带 SocketServer 指针）
Thread::Thread(SocketServer *socket_server)
    : socket_server_(socket_server), own_socket_server_(nullptr)
{
    SetName("Thread", this);
}

// 构造函数（带 SocketServer 所有权）
Thread::Thread(std::unique_ptr<SocketServer> socket_server)
    : socket_server_(socket_server.get()), own_socket_server_(std::move(socket_server))
{
    SetName("Thread", this);
}

// 析构函数
Thread::~Thread()
{
    Stop();
}

// 设置线程名称
bool Thread::SetName(const std::string &name, const void *obj)
{
    if (running_.load())
    {
        return false;
    }

    name_ = name;
    if (obj)
    {
        std::ostringstream oss;
        oss << " 0x" << std::hex << std::setw(std::numeric_limits<void *>::digits / 4 + 1)
            << std::setfill('0') << reinterpret_cast<uintptr_t>(obj);
        name_ += oss.str();
    }
    return true;
}

// 启动线程
bool Thread::Start()
{
    if (running_.load())
    {
        return false;
    }

    stop_flag_.store(false);
    thread_ = std::thread(&Thread::ThreadFunc, this);
    running_.store(true);
    return true;
}

// 停止线程
void Thread::Stop()
{
    if (!running_.load())
    {
        return;
    }

    // 设置停止标志
    stop_flag_.store(true);

    // 唤醒线程（如果有任务在等待）
    task_cv_.notify_all();

    // 等待线程结束
    if (thread_.joinable())
    {
        thread_.join();
    }

    running_.store(false);

    // 清空剩余任务
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        while (!task_queue_.empty())
        {
            task_queue_.pop();
        }
    }

    // 清空延迟任务
    {
        std::lock_guard<std::mutex> lock(delayed_mutex_);
        while (!delayed_queue_.empty())
        {
            delayed_queue_.pop();
        }
    }
}

// 投递任务到线程
void Thread::PostTask(std::unique_ptr<QueuedTask> task)
{
    if (!task)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        task_queue_.push(std::move(task));
    }

    task_cv_.notify_one();
}

// 投递延迟任务
void Thread::PostDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds)
{
    if (!task)
    {
        return;
    }

    DelayedTask delayed_task;
    delayed_task.run_time_ms = GetCurrentTimeMs() + milliseconds;
    delayed_task.sequence = delayed_sequence_++;
    delayed_task.task = std::move(task);

    {
        std::lock_guard<std::mutex> lock(delayed_mutex_);
        delayed_queue_.push(std::move(delayed_task));
    }

    task_cv_.notify_one();
}

// 线程主函数
void Thread::ThreadFunc()
{
    // 运行派生类的 Run 方法
    Run();
}

// 运行线程（处理任务队列）
void Thread::Run()
{
    while (!stop_flag_.load())
    {
        ProcessTask();
    }
}

// 获取当前时间（毫秒）
int64_t Thread::GetCurrentTimeMs() const
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// 处理任务
void Thread::ProcessTask()
{
    std::unique_ptr<QueuedTask> task;

    {
        std::unique_lock<std::mutex> lock(task_mutex_);

        // 检查是否有立即任务或延迟任务到期
        bool has_ready_task = !task_queue_.empty();

        if (!has_ready_task)
        {
            // 检查延迟任务
            std::lock_guard<std::mutex> delayed_lock(delayed_mutex_);
            if (!delayed_queue_.empty())
            {
                int64_t current_time = GetCurrentTimeMs();
                const DelayedTask &top = delayed_queue_.top();
                if (top.run_time_ms <= current_time)
                {
                    has_ready_task = true;
                    // 移动延迟任务到立即任务队列
                    DelayedTask delayed = const_cast<DelayedTask &>(delayed_queue_.top());
                    delayed_queue_.pop();
                    task = std::move(delayed.task);
                }
            }
        }

        if (has_ready_task)
        {
            if (!task && !task_queue_.empty())
            {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        else
        {
            // 计算等待时间
            int64_t wait_ms = 100; // 默认等待 100ms

            {
                std::lock_guard<std::mutex> delayed_lock(delayed_mutex_);
                if (!delayed_queue_.empty())
                {
                    int64_t current_time = GetCurrentTimeMs();
                    const DelayedTask &top = delayed_queue_.top();
                    wait_ms = top.run_time_ms - current_time;
                    if (wait_ms < 0)
                    {
                        wait_ms = 0;
                    }
                }
            }

            // 等待任务或超时
            task_cv_.wait_for(lock, std::chrono::milliseconds(wait_ms), [this]
                              { return !task_queue_.empty() || stop_flag_.load(); });

            // 如果被唤醒，再次检查
            if (stop_flag_.load() && task_queue_.empty())
            {
                return;
            }

            if (!task_queue_.empty())
            {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
            else
            {
                // 再次检查延迟任务
                std::lock_guard<std::mutex> delayed_lock(delayed_mutex_);
                if (!delayed_queue_.empty())
                {
                    int64_t current_time = GetCurrentTimeMs();
                    const DelayedTask &top = delayed_queue_.top();
                    if (top.run_time_ms <= current_time)
                    {
                        DelayedTask delayed = const_cast<DelayedTask &>(delayed_queue_.top());
                        delayed_queue_.pop();
                        task = std::move(delayed.task);
                    }
                }
            }
        }
    }

    if (task)
    {
        // 执行任务
        bool delete_task = task->Run();
        if (delete_task)
        {
            task.reset();
        }
        else
        {
            // 任务返回 false 表示接管所有权
            task.release();
        }
    }
}

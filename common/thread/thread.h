#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// SocketServer 前向声明（简化版本）
class SocketServer;

// QueuedTask 接口 - 兼容 WebRTC api/queued_task.h
class QueuedTask
{
public:
    virtual ~QueuedTask() = default;

    // 执行任务
    // 返回 true: 任务完成后应被删除
    // 返回 false: 任务接管所有权，不应被删除
    virtual bool Run() = 0;
};

// 辅助函数：将 lambda/函数转换为 QueuedTask
// 兼容 webrtc::ToQueuedTask
template <typename FunctorT>
std::unique_ptr<QueuedTask> ToQueuedTask(FunctorT &&functor)
{
    class TaskImpl : public QueuedTask
    {
    public:
        explicit TaskImpl(FunctorT &&functor)
            : functor_(std::forward<FunctorT>(functor)) {}

        bool Run() override
        {
            functor_();
            return true;
        }

    private:
        std::decay_t<FunctorT> functor_;
    };

    return std::make_unique<TaskImpl>(std::forward<FunctorT>(functor));
}

// Thread 类 - 兼容 WebRTC Thread API
class Thread
{
public:
    // 创建线程
    static std::unique_ptr<Thread> Create();

    // 创建带 SocketServer 的线程
    static std::unique_ptr<Thread> CreateWithSocketServer();

    // 析构函数
    virtual ~Thread();

    // 设置线程名称（必须在 Start 之前调用）
    bool SetName(const std::string &name, const void *obj);

    // 启动线程
    bool Start();

    // 停止线程
    void Stop();

    // 投递任务到线程（异步执行）
    void PostTask(std::unique_ptr<QueuedTask> task);

    // 投递延迟任务
    void PostDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds);

    // 获取线程名称
    const std::string &name() const { return name_; }

    // 检查线程是否正在运行
    bool RunningForTest() const { return running_.load(); }

protected:
    // 构造函数
    explicit Thread(SocketServer *socket_server);
    explicit Thread(std::unique_ptr<SocketServer> socket_server);

    // 线程运行函数
    virtual void Run();

private:
    // 延迟任务结构
    struct DelayedTask
    {
        int64_t run_time_ms;
        uint32_t sequence;
        std::unique_ptr<QueuedTask> task;

        bool operator<(const DelayedTask &other) const
        {
            if (run_time_ms == other.run_time_ms)
            {
                return sequence > other.sequence;
            }
            return run_time_ms > other.run_time_ms;
        }
    };

    // 线程主函数
    void ThreadFunc();

    // 处理任务
    void ProcessTask();

    // 获取当前时间（毫秒）
    int64_t GetCurrentTimeMs() const;

    SocketServer *socket_server_;
    std::unique_ptr<SocketServer> own_socket_server_;

    std::string name_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_flag_{false};

    std::thread thread_;

    // 任务队列
    std::queue<std::unique_ptr<QueuedTask>> task_queue_;
    mutable std::mutex task_mutex_;
    std::condition_variable task_cv_;

    // 延迟任务队列
    std::priority_queue<DelayedTask> delayed_queue_;
    std::mutex delayed_mutex_;
    std::atomic<uint32_t> delayed_sequence_{0};
};

#endif // THREAD_H

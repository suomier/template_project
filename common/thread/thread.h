#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#include "types/function_view.hpp"

class Thread
{
public:
    static std::shared_ptr<Thread> Create();

    bool SetName(const std::string &name, uint32_t index);
    const std::string &getName() const;

    bool Start();
    void Stop();

    //

    /**
     * @brief 便捷方法，在另一个线程上调用 functor
     * @tparam ReturnT 返回类型
     * @param posted_from 调用位置
     * @param functor 要调用的函数
     * @return 函数的返回值
     * @details 调用者必须提供 `ReturnT` 模板参数，该参数不能（轻易）被推断。
     * 在内部使用 Send()，它会阻塞当前线程直到执行完成。
     * 示例：bool result = thread.Invoke<bool>(RTC_FROM_HERE, &MyFunctionReturningBool);
     * 注意：此函数仅在允许同步调用时才能调用。有关详细信息，请参阅 ScopedDisallowBlockingCalls。
     * 注意：不鼓励阻塞调用，请考虑是否可以使用 PostTask() 和回调来实现您正在做的事情。
     */
    template <class ReturnT, typename = typename std::enable_if<!std::is_void<ReturnT>::value>::type>
    ReturnT Invoke(FunctionView<ReturnT()> functor)
    {
        ReturnT result;
        InvokeInternal([functor, &result]
                       { result = functor(); });

        return result;
    }

    template <class ReturnT, typename = typename std::enable_if<std::is_void<ReturnT>::value>::type>
    void Invoke(FunctionView<void()> functor)
    {
        InvokeInternal(functor);
    }

    /**
     * @brief 异步发布任务以在 `this` 线程上调用 functor
     * @tparam FunctorT 函数对象类型
     * @param posted_from 发布位置
     * @param functor 要调用的函数对象
     * @details 即不阻塞调用 PostTask() 的线程。`functor` 的所有权被传递，
     * 并在调用后（通常情况下，见下文）在 `this` 线程上销毁。
     *
     * FunctorT 的要求：
     * - FunctorT 是可移动的。
     * - FunctorT 实现某个 T 的 "T operator()()" 或 "T operator()() const"
     *   （如果 T 不是 void，返回值在 `this` 线程上被丢弃）。
     * - FunctorT 具有可以从 `this` 线程调用的公共析构函数，在调用 operation() 之后。
     * - functor 不能在 PostTask() 完成之前导致线程退出。
     *
     * functor/task 的销毁模仿 TaskQueue::PostTask 的行为：如果任务运行，
     * 它将在 `this` 线程上销毁。但是，如果在 Thread 被销毁时有挂起的任务，
     * 或者任务被发布到正在退出的线程，则任务会立即在调用线程上销毁。
     * 销毁 Thread 仅阻塞任何当前运行的任务完成。注意，TQ 抽象在这些情况下
     * 对销毁如何发生更加模糊，允许销毁在稍后的时间异步地在某个任意线程上发生。
     * 因此，为了便于迁移，不要依赖 Thread::PostTask 立即销毁未运行的任务。
     *
     * 示例 - 调用类方法：
     * class Foo {
     *  public:
     *   void DoTheThing();
     * };
     * Foo foo;
     * thread->PostTask(RTC_FROM_HERE, Bind(&Foo::DoTheThing, &foo));
     *
     * 示例 - 调用 lambda 函数：
     * thread->PostTask(RTC_FROM_HERE,
     *                  [&x, &y] { x.TrackComputations(y.Compute()); });
     */
    template <class FunctorT>
    void PostTask(FunctorT &&functor)
    {
        Post(posted_from, GetPostTaskMessageHandler(), /*id=*/0,
             new rtc_thread_internal::MessageWithFunctor<FunctorT>(
                 std::forward<FunctorT>(functor)));
    }

    /**
     * @brief 异步发布延迟任务以在 `this` 线程上调用 functor
     * @tparam FunctorT 函数对象类型
     * @param posted_from 发布位置
     * @param functor 要调用的函数对象
     * @param milliseconds 延迟时间（毫秒）
     * @details 不阻塞调用 PostDelayedTask() 的线程。`functor` 的所有权被传递，
     * 并在调用后在 `this` 线程上销毁。
     * functor 的所有权在 `milliseconds` 毫秒后调用。
     */
    template <class FunctorT>
    void PostDelayedTask( FunctorT &&functor, uint32_t milliseconds)
    {
        PostDelayed(posted_from, milliseconds, GetPostTaskMessageHandler(),
                    /*id=*/0,
                    new rtc_thread_internal::MessageWithFunctor<FunctorT>(
                        std::forward<FunctorT>(functor)));
    }

private:
    /**
     * @brief 内部调用方法
     * @param posted_from 调用位置
     * @param functor 要调用的函数
     */
    void InvokeInternal( FunctionView<void()> functor);

protected:
    virtual ~Thread();

    virtual void Run();

private:
};

#endif // THREAD_H

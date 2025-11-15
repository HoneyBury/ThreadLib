#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <type_traits>
#include "concurrent_queue.hpp"
namespace cppthreadflow {
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 禁止拷贝和移动，因为线程池是唯一的资源管理者
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    // 模板参数
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

private:
    // 工作线程的执行函数
    void worker_thread();

    std::vector<std::thread> workers_;
    ConcurrentQueue<std::function<void()>> task_queue_;
    std::atomic<bool> stop_flag_{false};
};


template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    if (stop_flag_) {
        throw std::runtime_error("submit on a stopped ThreadPool");
    }

    using return_type = std::invoke_result_t<F, Args...>;

    // 使用 packaged_task 包装任务，以便获取 future
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        // 使用完美转发绑定参数
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> future = task->get_future();

    // 将任务的执行体（lambda）放入队列
    task_queue_.push([task]() { (*task)(); });

    return future;
}

} // namespace cppthreadflow
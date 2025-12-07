#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

#include "concurrent_queue.hpp"
#include "work_stealing_queue.hpp"

namespace cppthreadflow {

class WorkStealingThreadPool {
 public:
  explicit WorkStealingThreadPool(
      size_t num_threads = std::thread::hardware_concurrency());
  ~WorkStealingThreadPool();

  WorkStealingThreadPool(const WorkStealingThreadPool&) = delete;
  WorkStealingThreadPool& operator=(const WorkStealingThreadPool&) = delete;

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>>;

 private:
  void worker_thread(size_t index);
  std::optional<Task> try_steal(size_t my_index);

  // 本地队列集合
  std::vector<std::unique_ptr<WorkStealingQueue>> queues_;
  // 全局队列（用于非工作线程提交）
  ConcurrentQueue<Task> global_queue_;

  std::vector<std::thread> workers_;
  std::atomic<bool> stop_flag_{false};

  // 线程局部变量：指示当前线程在池中的索引，-1 表示非工作线程
  inline static thread_local size_t my_index_ = static_cast<size_t>(-1);

  // 引入一个睡眠同步等待的机制，当不断yied一段时间都没有任务就直接阻塞等待 ，这样能有效降低cpu的占用
  std::mutex sleep_mutex_;
  std::condition_variable sleep_cv_;
  std::atomic<size_t> idle_threads_{0};
};

template <class F, class... Args>
auto WorkStealingThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
  using return_type = std::invoke_result_t<F, Args...>;

  // 1. 封装任务
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      [func = std::forward<F>(f),
       args = std::make_tuple(std::forward<Args>(args)...)]() {
        return std::apply(func, args);
      });

  std::future<return_type> res = task->get_future();
  Task wrapper = [task]() { (*task)(); };

  // 2. 路由任务
  if (my_index_ != static_cast<size_t>(-1)) {
    // 如果我是本池子的工作线程，放入我的本地队列 (LIFO)
    // 注意：这里简单假设 my_index_ 对应 queues_ 范围内的索引
    if (my_index_ < queues_.size()) {
      queues_[my_index_]->push(std::move(wrapper));
    } else {
      global_queue_.push(std::move(wrapper));
    }
  } else {
    // 外部线程（如主线程）提交，放入全局队列
    global_queue_.push(std::move(wrapper));
  }
  // 只有当有线程空闲（睡觉或自旋中）时，才发出通知
  // 这是一个关键优化，避免惊群效应和系统调用开销
  if (idle_threads_.load(std::memory_order_relaxed) > 0) {
    sleep_cv_.notify_one();
  }
  return res;
}

}  // namespace cppthreadflow
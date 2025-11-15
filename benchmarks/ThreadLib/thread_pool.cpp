#include "thread_pool.hpp"

namespace cppthreadflow {

ThreadPool::ThreadPool(size_t num_threads) {
 if (num_threads == 0) {
  // 保证至少有一个线程
  num_threads = 1;
 }
 workers_.reserve(num_threads);
 for (size_t i = 0; i < num_threads; ++i) {
  // 创建并启动工作线程
  workers_.emplace_back(&ThreadPool::worker_thread, this);
 }
}

ThreadPool::~ThreadPool() {
 // 1. 设置停止标志
 stop_flag_.store(true);

 // 2. 停止任务队列，唤醒所有可能在等待任务的线程
 task_queue_.stop();

 // 3. 等待所有工作线程执行完毕并退出
 for (std::thread& worker : workers_) {
  if (worker.joinable()) {
   worker.join();
  }
 }
}

void ThreadPool::worker_thread() {
 while (true) {
  std::function<void()> task;

  // 从任务队列中获取任务，如果队列为空则阻塞
  if (!task_queue_.pop(task)) {
   // 如果 pop 返回 false，意味着队列已停止且为空，线程可以安全退出
   return;
  }

  // 执行任务
  if (task) {
   task();
  }
 }
}

} // namespace cppthreadflow
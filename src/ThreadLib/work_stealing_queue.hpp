#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <functional>

namespace cppthreadflow {

using Task = std::function<void()>;

/**
 * @brief 线程本地的工作窃取队列。
 * 支持 LIFO (后进先出) 的本地操作和 FIFO (先进先出) 的窃取操作。
 */
class WorkStealingQueue {
public:
  WorkStealingQueue() = default;

  // 禁止拷贝和移动
  WorkStealingQueue(const WorkStealingQueue&) = delete;
  WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

  /**
   * @brief [仅限拥有者线程] 向队列头部推入任务。
   */
  void push(Task task) {
    std::lock_guard<std::mutex> lock(mutex_);
    deque_.push_front(std::move(task));
  }

  /**
   * @brief [仅限拥有者线程] 从队列头部弹出任务 (LIFO)。
   */
  std::optional<Task> pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (deque_.empty()) {
      return std::nullopt;
    }
    Task task = std::move(deque_.front());
    deque_.pop_front();
    return task;
  }

  /**
   * @brief [其他线程] 从队列尾部窃取任务 (FIFO)。
   */
  std::optional<Task> steal() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (deque_.empty()) {
      return std::nullopt;
    }
    Task task = std::move(deque_.back());
    deque_.pop_back();
    return task;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deque_.empty();
  }

private:
  std::deque<Task> deque_;
  mutable std::mutex mutex_;
};

} // namespace cppthreadflow
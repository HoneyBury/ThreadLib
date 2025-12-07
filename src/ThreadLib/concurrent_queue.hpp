#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
namespace cppthreadflow {

/**
 * @brief 一个基础的线程安全阻塞队列。
 *
 * @tparam T 队列中存储的元素类型。
 */
template <typename T>
class ConcurrentQueue {
 public:
  ConcurrentQueue() = default;
  ~ConcurrentQueue() = default;

  // 禁止拷贝构造和赋值
  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

  /**
   * @brief 向队列尾部添加一个元素。
   * @param item 要添加的元素，将通过移动语义传入。
   */
  void push(T item) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      queue_.push(std::move(item));
    }  // 提前释放锁，再通知，以减少锁的持有时间
    cond_.notify_one();
  }

  /**
   * @brief
   * 从队列头部弹出一个元素。如果队列为空，此操作将阻塞，直到队列中有元素或队列被停止。
   * @param item 用于接收弹出元素的引用。
   * @return 如果成功弹出一个元素，返回 true；如果队列被停止且为空，返回 false。
   */
  bool pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    // 等待直到队列不为空或队列被停止
    cond_.wait(lock, [this] { return !queue_.empty() || stop_; });

    // 如果因停止信号而被唤醒，且队列已空，则不再弹出元素
    if (stop_ && queue_.empty()) {
      return false;
    }

    item = std::move(queue_.front());
    queue_.pop();
    return true;
  }

  /**
   * @brief 停止队列。
   * 这将唤醒所有因等待元素而阻塞的线程。一旦队列被停止，pop操作将在队列为空时立即返回false。
   */
  void stop() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cond_.notify_all();
  }
  /**
       * @brief 尝试从队列弹出元素（非阻塞）。
       * @param item 用于接收元素的引用。
       * @return 如果成功弹出返回 true，队列为空则立即返回 false。
       */
  bool try_pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    item = std::move(queue_.front());
    queue_.pop();
    return true;
  }
 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool stop_ = false;
};

}  // namespace cppthreadflow
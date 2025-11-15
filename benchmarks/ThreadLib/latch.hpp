#pragma once

#include <condition_variable>
#include <mutex>

namespace cppthreadflow {

/**
 * @brief 一个 C++17 实现的门闩 (Latch)。
 *
 * 门闩是一个一次性的同步原语。
 * 线程可以通过调用 wait() 来阻塞，直到内部计数器减到 0。
 * 当计数器到 0 时，所有等待的线程都被释放，门闩永久保持打开状态。
 */
class Latch {
 public:
  /**
   * @brief 构造一个门闩。
   * @param initial_count 初始计数值。当计数器从这个值减到 0 时，门闩打开。
   */
  explicit Latch(int initial_count);

  // 门闩是不可拷贝、不可移动的资源
  Latch(const Latch&) = delete;
  Latch& operator=(const Latch&) = delete;
  Latch(Latch&&) = delete;
  Latch& operator=(Latch&&) = delete;

  /**
   * @brief 使内部计数器减 1。
   *
   * 如果此次操作使计数器达到 0，则唤醒所有正在 wait() 的线程。
   * 如果计数器已经是 0，此操作无效果。
   */
  void count_down();

  /**
   * @brief 阻塞当前线程，直到门闩的计数器达到 0。
   *
   * 如果调用此函数时计数器已经是 0，则立即返回，不阻塞。
   */
  void wait() const;

 private:
  int count_;
  // 关键：mutex 和 cv 必须是 mutable，
  // 这样我们才能在 const 成员函数 wait() 中调用它们。
  // wait() 不会改变 Latch 的逻辑状态（计数），因此它应该是 const 的。
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};

}  // namespace cppthreadflow
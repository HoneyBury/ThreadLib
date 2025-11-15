#pragma once

#include <condition_variable>
#include <mutex>

namespace cppthreadflow {

/**
 * @brief 一个 C++17 实现的可重用屏障 (Barrier)。
 *
 * 屏障是一个同步原语，它允许多个线程在某个点上集合，
 * 直到所有线程都到达该点，它们才会被同时释放。
 * 此屏障是可重用的（循环的）。
 */
class Barrier {
 public:
  /**
   * @brief 构造一个屏障。
   * @param party_count 参与者的数量。必须大于 0。
   */
  explicit Barrier(int party_count);

  // 屏障是不可拷贝、不可移动的资源
  Barrier(const Barrier&) = delete;
  Barrier& operator=(const Barrier&) = delete;
  Barrier(Barrier&&) = delete;
  Barrier& operator=(Barrier&&) = delete;

  /**
   * @brief 到达屏障并阻塞，直到所有参与者都到达。
   *
   * 当所有参与者都调用了此函数后，所有线程将被释放，
   * 屏障将自动重置为下一轮使用。
   */
  void arrive_and_wait();

 private:
  const int party_count_;  // 参与者总数
  int current_count_;      // 当前代已到达的数量
  int generation_;         // 当前的“代”
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace cppthreadflow
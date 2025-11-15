#pragma once

#include <condition_variable>
#include <mutex>

namespace cppthreadflow {

/**
 * @brief 一个 C++17 实现的计数信号量。
 *
 * 信号量是用于控制对共享资源访问的并发原语。
 */
class Semaphore {
 public:
  /**
   * @brief 构造一个信号量。
   * @param initial_count 信号量的初始计数值，默认为0。
   */
  explicit Semaphore(int initial_count = 0);

  // 禁止拷贝和移动
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;
  Semaphore(Semaphore&&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;

  /**
   * @brief 释放一个信号量资源（V操作）。
   *
   * 信号量计数加1，并唤醒一个可能在 acquire() 上等待的线程。
   */
  void release();

  /**
   * @brief 获取一个信号量资源（P操作）。
   *
   * 如果信号量计数大于0，则计数减1并立即返回。
   * 如果信号量计数为0，则阻塞当前线程，直到计数变为正数。
   */
  void acquire();

  /**
   * @brief 尝试非阻塞地获取一个信号量资源。
   * @return 如果获取成功（计数大于0），返回 true。
   * 如果获取失败（计数为0），立即返回 false 而不阻塞。
   */
  bool try_acquire();

 private:
  int count_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace cppthreadflow
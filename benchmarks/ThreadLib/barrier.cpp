#include "barrier.hpp"
#include <stdexcept>

namespace cppthreadflow {

Barrier::Barrier(int party_count)
    : party_count_(party_count), current_count_(0), generation_(0)
{
  if (party_count <= 0) {
    throw std::invalid_argument("Barrier party count must be positive.");
  }
}

void Barrier::arrive_and_wait() {
  std::unique_lock<std::mutex> lock(mutex_);

  // 记录下我进入的是哪一代
  const int my_generation = generation_;

  // 当前代计数 + 1
  current_count_++;

  if (current_count_ == party_count_) {
    // 我是最后一个到达的线程

    // 1. 将代推进到下一代
    generation_++;

    // 2. 将当前计数器重置为 0，为下一代做准备
    current_count_ = 0;

    // 3. 释放锁，并唤醒所有在这一代等待的同伴
    lock.unlock();
    cv_.notify_all();

    // 作为最后一个线程，我不需要等待，直接返回
  } else {
    // 我不是最后一个，我必须等待

    // 4. 使用谓词安全等待，直到“代”发生变化
    //    这确保了：
    //    a) 我们只在“代”真正推进时才被唤醒（防止虚假唤醒）
    //    b) 我们只会被“这一代”的最后一个线程所唤醒
    cv_.wait(lock, [this, my_generation] {
        return generation_ != my_generation;
    });

    // 唤醒后，锁被重新获取，我们退出函数，锁会自动释放
  }
}

} // namespace cppthreadflow
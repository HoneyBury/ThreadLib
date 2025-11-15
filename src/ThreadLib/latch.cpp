#include "latch.hpp"

namespace cppthreadflow {

Latch::Latch(int initial_count) : count_(initial_count) {
  // 我们可以添加一个检查，如果 initial_count <= 0，
  // 那么门闩被认为是“天生打开的”。
  if (initial_count < 0) {
    count_ = 0; // 计数值不能为负
  }
}

void Latch::count_down() {
  std::unique_lock<std::mutex> lock(mutex_);

  // 只有在计数器尚未归零时才执行操作
  if (count_ > 0) {
    count_--;
    if (count_ == 0) {
      // 计数器达到 0，这是关键时刻。
      // 释放锁并唤醒所有等待的线程。
      // 尽早释放锁是一种优化，以避免被唤醒的线程
      // 立即尝试获取同一个锁而导致短暂的竞争。
      lock.unlock();
      cv_.notify_all(); // 必须是 notify_all()
    }
  }
  // 如果 count_ 已经是 0，则什么也不做。
}

void Latch::wait() const {
  std::unique_lock<std::mutex> lock(mutex_);

  // 检查计数器是否已经为 0
  if (count_ == 0) {
    return; // 门闩已打开，立即返回
  }

  // 使用 wait() 和谓词 lambda 来安全地等待
  // 谓词会处理“虚假唤醒” (spurious wakeups)
  // 只有当 count_ == 0 时，wait() 才会返回
  cv_.wait(lock, [this] { return count_ == 0; });
}

} // namespace cppthreadflow
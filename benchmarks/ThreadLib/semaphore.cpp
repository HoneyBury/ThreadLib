#include "semaphore.hpp"

namespace cppthreadflow {

Semaphore::Semaphore(int initial_count) : count_(initial_count) {
  // 构造函数体
  // initial_count 必须是非负数，但在一个库中，我们也可以选择
  // 使用断言(assert)或抛出异常来强制约束。
  // 为简单起见，我们暂且信任用户输入。
}

void Semaphore::release() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // 信号量计数增加
    count_++;
  }  // 尽早释放锁

  // 唤醒一个正在等待的线程
  // 在锁之外调用 notify_one() 是一种优化，
  // 它可以避免被唤醒的线程立即尝试获取锁时又被阻塞。
  cv_.notify_one();
}

void Semaphore::acquire() {
  std::unique_lock<std::mutex> lock(mutex_);

  // 使用 wait() 和 lambda 谓词来安全地等待
  // Lambda 谓词是防止“虚假唤醒” (spurious wakeup) 的关键
  cv_.wait(lock, [this] { return count_ > 0; });

  // 成功获取，计数减1
  count_--;
}

bool Semaphore::try_acquire() {
  std::unique_lock<std::mutex> lock(mutex_);

  if (count_ > 0) {
    // 资源可用，获取它
    count_--;
    return true;
  }

  // 资源不可用，立即返回
  return false;
}

}  // namespace cppthreadflow
#include "work_stealing_thread_pool.hpp"

#include <random>

namespace cppthreadflow {

WorkStealingThreadPool::WorkStealingThreadPool(size_t num_threads) {
  if (num_threads == 0) num_threads = 1;

  // 1. 创建本地队列
  for (size_t i = 0; i < num_threads; ++i) {
    queues_.push_back(std::make_unique<WorkStealingQueue>());
  }

  // 2. 启动工作线程
  workers_.reserve(num_threads);
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this, i] {
      my_index_ = i;  // 设置线程局部索引
      worker_thread(i);
    });
  }
}

WorkStealingThreadPool::~WorkStealingThreadPool() {
  stop_flag_.store(true);
  // 唤醒可能阻塞在全局队列上的线程
  global_queue_.stop();
  sleep_cv_.notify_all();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void WorkStealingThreadPool::worker_thread(size_t index) {
  // 自旋次数阈值，经验值，可以根据 CPU 频率调整
  // 4000 次空循环大约消耗几个微秒
  const int SPIN_COUNT_THRESHOLD = 4000;
  int spin_count = 0;
  while (!stop_flag_) {
    std::optional<Task> task;

    // 策略 1: 优先处理本地队列 (LIFO) - 无竞争/低竞争
    task = queues_[index]->pop();
    if (task) {
      (*task)();
      spin_count = 0;  // 重置自旋计数
      continue;
    }

    // 策略 2: 本地为空，尝试全局队列
    Task global_task;
    if (global_queue_.try_pop(global_task)) {
      spin_count = 0;  // 重置自旋
      global_task();
      continue;
    }

    // 策略 3: 全局也为空，尝试窃取 (Steal)
    task = try_steal(index);
    if (task) {
      spin_count = 0;  // 重置自旋
      (*task)();
      continue;
    }

    // 策略 4: 实在没活干了
    // 这里为了避免 CPU 100% 空转，我们可以选择休眠或出让时间片。
    // 在高性能实现中，这里需要结合 Condition Variable 进行自旋+休眠。
    // --- 自适应睡眠逻辑 ---

    spin_count++;

    if (spin_count < SPIN_COUNT_THRESHOLD) {
      // 阶段一：自旋
      // yield 提示 CPU 暂时出让时间片，但线程仍处于就绪状态
      std::this_thread::yield();
    } else {
      // 阶段二：休眠 (Park)
      // 自旋很久都没任务，进入省电模式

      spin_count = 0;  // 重置计数，醒来后重新开始自旋周期

      // 标记自己为空闲
      idle_threads_.fetch_add(1, std::memory_order_relaxed);

      {
        std::unique_lock<std::mutex> lock(sleep_mutex_);
        // 这里使用 wait_for 而不是 wait 是一个防御性编程技巧。
        // 雖然我們有 notify，但在極端併發下可能會出現"丟失喚醒"(Lost
        // Wakeup)的情況， 或者任務被偷走但 notify 沒觸發。
        // 設置一個短暫的超時（例如 10ms），保證線程就算沒被喚醒，
        // 也會定期醒來檢查一下有沒有活幹。這比死等更健壯。
        sleep_cv_.wait_for(lock, std::chrono::milliseconds(10), [this] {
          return stop_flag_
              .load();  // 謂詞：如果停止了就醒來，否則依賴超時或notify
        });
      }

      // 醒来了，标记自己不再空闲
      idle_threads_.fetch_sub(1, std::memory_order_relaxed);
    }
  }
}

std::optional<Task> WorkStealingThreadPool::try_steal(size_t my_index) {
  size_t num_queues = queues_.size();
  if (num_queues <= 1) return std::nullopt;

  // 随机选择
  static thread_local std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, num_queues - 1);

  size_t start_index = dist(gen);

  // 遍历所有其他队列尝试窃取
  for (size_t i = 0; i < num_queues; ++i) {
    size_t target_index = (start_index + i) % num_queues;
    if (target_index == my_index) continue;

    auto task = queues_[target_index]->steal();
    if (task) {
      return task;
    }
  }

  return std::nullopt;
}

}  // namespace cppthreadflow
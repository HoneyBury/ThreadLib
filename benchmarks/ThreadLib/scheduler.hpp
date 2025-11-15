#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace cppthreadflow {

// 前向声明，避免循环引用头文件
class ThreadPool;

/**
 * @brief 一个任务调度器，用于执行延迟或周期性任务。
 */
class Scheduler {
 public:
  // 为 chrono 类型创建更易读的别名
  using Task = std::function<void()>;
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  /**
   * @brief 构造函数。
   * @param pool 一个线程池的引用，所有到期的任务将提交到这个池中执行。
   */
  explicit Scheduler(ThreadPool& pool);

  /**
   * @brief 析构函数。
   * 将安全地停止调度器线程。
   */
  ~Scheduler();

  // 禁止拷贝和移动
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
  Scheduler(Scheduler&&) = delete;
  Scheduler& operator=(Scheduler&&) = delete;

  /**
   * @brief 在指定的时间点执行一次任务。
   * @param time 任务执行的绝对时间点。
   * @param task 要执行的任务。
   */
  void schedule_at(const TimePoint& time, Task task);

  /**
   * @brief 在指定的延迟后执行一次任务。
   * @param delay 相对于现在的延迟时间。
   * @param task 要执行的任务。
   */
  void schedule_after(const Duration& delay, Task task);

  /**
   * @brief 安排一个周期性任务。
   * @param first_time 第一次执行的绝对时间点。
   * @param interval 两次执行之间的时间间隔。
   * @param task 要周期性执行的任务。
   */
  void schedule_periodic(const TimePoint& first_time, const Duration& interval,
                         Task task);

 private:
  // 内部用于存储任务的结构体
  struct ScheduledTask {
    TimePoint time;
    Duration interval;  // 对于非周期性任务，此值为0
    Task func;
  };

  // 用于优先队列的比较器，时间早的优先级高
  struct TaskComparer {
    bool operator()(const ScheduledTask& a, const ScheduledTask& b) const {
      return a.time > b.time;
    }
  };

  // 调度器主循环
  void scheduler_loop();

  ThreadPool& pool_;
  std::thread scheduler_thread_;
  std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, TaskComparer>
      tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> stop_{false};
};

}  // namespace cppthreadflow
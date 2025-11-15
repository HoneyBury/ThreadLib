#include "scheduler.hpp"
#include "thread_pool.hpp" // 需要 ThreadPool 的完整定义

namespace cppthreadflow {

Scheduler::Scheduler(ThreadPool& pool) : pool_(pool) {
    // 启动调度器线程，并将主循环函数作为入口
    scheduler_thread_ = std::thread(&Scheduler::scheduler_loop, this);
}

Scheduler::~Scheduler() {
    // 1. 设置停止标志
    stop_.store(true);
    // 2. 唤醒可能正在休眠的调度线程
    cv_.notify_one();
    // 3. 等待线程执行完毕
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
}

void Scheduler::schedule_at(const TimePoint& time, Task task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push({time, Duration::zero(), std::move(task)});
    }
    // 通知调度线程，可能有新的、更早的任务需要处理
    cv_.notify_one();
}

void Scheduler::schedule_after(const Duration& delay, Task task) {
    schedule_at(Clock::now() + delay, std::move(task));
}

void Scheduler::schedule_periodic(const TimePoint& first_time, const Duration& interval, Task task) {
    if (interval == Duration::zero()) {
        // 避免无限循环
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push({first_time, interval, std::move(task)});
    }
    cv_.notify_one();
}

void Scheduler::scheduler_loop() {
    while (!stop_) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (tasks_.empty()) {
            // 如果任务队列为空，则无限期等待，直到被唤醒（新任务或析构）
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        } else {
            // 队列不为空，则等待直到下一个任务的时间点，或被唤醒
            auto next_time = tasks_.top().time;
            cv_.wait_until(lock, next_time, [this, next_time] {
                // 仅在以下情况被真正唤醒：
                // 1. 停止信号来了
                // 2. 任务队列为空了（可能被其他线程偷走，虽然我们这里没有）
                // 3. 队首任务变了（即有新任务插入，且比当前队首任务更早）
                return stop_ || tasks_.empty() || tasks_.top().time < next_time;
            });
        }

        // 如果是被析构函数唤醒并设置了停止位，则直接退出循环
        if (stop_) {
            break;
        }

        // 检查是否有到期任务
        while (!tasks_.empty() && tasks_.top().time <= Clock::now()) {
            // 从优先队列中取出任务
            ScheduledTask scheduled_task = tasks_.top();
            tasks_.pop();

            // 【关键】提前释放锁，再去提交任务
            lock.unlock();

            // 将任务提交到线程池执行
            pool_.submit(scheduled_task.func);

            // 重新加锁以处理周期性任务和循环
            lock.lock();

            // 如果是周期性任务，计算下一次执行时间并重新入队
            if (scheduled_task.interval > Duration::zero()) {
                scheduled_task.time += scheduled_task.interval;
                tasks_.push(scheduled_task);
            }
        }
    }
}

} // namespace cppthreadflow
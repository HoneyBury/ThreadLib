#include <gtest/gtest.h>
#include "ThreadLib/scheduler.hpp"
#include "ThreadLib/thread_pool.hpp"
#include <chrono>
#include <future>
#include <atomic>
#include <vector>
#include <mutex>
#include <algorithm>

using namespace std::chrono_literals;

class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试用例开始前，都会创建一个新的线程池和调度器
        pool = std::make_unique<cppthreadflow::ThreadPool>(4);
        scheduler = std::make_unique<cppthreadflow::Scheduler>(*pool);
    }

    void TearDown() override {
        // 测试用例结束后，对象会被自动销毁，后台线程会安全退出
    }

    // 使用 unique_ptr 来自动管理对象的生命周期
    std::unique_ptr<cppthreadflow::ThreadPool> pool;
    std::unique_ptr<cppthreadflow::Scheduler> scheduler;
};

// 1. 测试单次延迟任务 (`schedule_after`) 的执行
TEST_F(SchedulerTest, ScheduleAfterExecutesTask) {
    using Clock = cppthreadflow::Scheduler::Clock;
    std::promise<Clock::time_point> promise;
    auto future = promise.get_future();

    auto start_time = Clock::now();
    auto delay = 200ms;

    // 安排一个任务，在执行时记录时间点
    scheduler->schedule_after(delay, [&promise]() {
        promise.set_value(Clock::now());
    });

    // 阻塞等待，直到任务执行并通过 promise 返回
    auto end_time = future.get();

    // 验证：任务执行的时间点与开始时间点的差值，应该大于等于指定的延迟
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_GE(elapsed.count(), delay.count());
}

// 2. 测试任务是否按照时间顺序执行（而不是提交顺序）
TEST_F(SchedulerTest, TasksExecuteInTemporalOrder) {
    std::promise<void> all_tasks_done_promise;
    auto all_tasks_done_future = all_tasks_done_promise.get_future();

    std::vector<int> execution_order;
    std::mutex vector_mutex;
    std::atomic<int> tasks_completed = 0;

    // 按 200ms -> 100ms 的顺序提交任务
    scheduler->schedule_after(200ms, [&]() {
        std::lock_guard<std::mutex> lock(vector_mutex);
        execution_order.push_back(2); // 任务2
        if (++tasks_completed == 2) all_tasks_done_promise.set_value();
    });

    scheduler->schedule_after(100ms, [&]() {
        std::lock_guard<std::mutex> lock(vector_mutex);
        execution_order.push_back(1); // 任务1
        if (++tasks_completed == 2) all_tasks_done_promise.set_value();
    });

    // 等待两个任务都完成
    all_tasks_done_future.wait_for(1s); // 设置一个超时以防万一

    // 验证：执行顺序应该是 [1, 2] (100ms 的任务先执行)
    ASSERT_EQ(execution_order.size(), 2);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
}

// 3. 测试周期性任务的执行
TEST_F(SchedulerTest, PeriodicTaskExecutesMultipleTimes) {
    std::atomic<int> execution_count = 0;
    auto interval = 100ms;

    // 安排一个立即开始、每100ms执行一次的周期性任务
    scheduler->schedule_periodic(cppthreadflow::Scheduler::Clock::now(), interval, [&]() {
        execution_count++;
    });

    // 等待一段时间，足够让任务执行多次
    // 等待 550ms，理论上应该执行 6 次 (0ms, 100ms, 200ms, 300ms, 400ms, 500ms)
    std::this_thread::sleep_for(550ms);

    // 停止调度器以停止周期性任务的进一步执行
    // 我们通过销毁 scheduler 对象来隐式停止
    scheduler.reset();

    // 验证：由于系统调度的不确定性，我们只验证一个合理的范围
    int count = execution_count.load();
    EXPECT_GE(count, 5);
    EXPECT_LE(count, 7);
}

// 4. 测试调度器析构时的优雅关闭
TEST_F(SchedulerTest, ShutdownIsGraceful) {
    // 创建一个新的调度器实例，以便我们可以控制其生命周期
    auto local_scheduler = std::make_unique<cppthreadflow::Scheduler>(*pool);

    // 安排一个在遥远未来的任务
    local_scheduler->schedule_after(10s, []() {
        // 这个任务不应该被执行
        FAIL() << "Task scheduled far in the future should not execute during shutdown.";
    });

    // 立即销毁调度器
    local_scheduler.reset();

    // 如果 reset() 操作能够顺利完成而不阻塞或崩溃，则测试通过。
    SUCCEED();
}
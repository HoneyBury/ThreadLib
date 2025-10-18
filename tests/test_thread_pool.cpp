#include <gtest/gtest.h>
#include "../src/ThreadLib/thread_pool.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <type_traits>
// 测试基本任务提交和结果获取
TEST(ThreadPoolTest, SubmitTaskAndGetResult) {
    cppthreadflow::ThreadPool pool(2);
    auto future = pool.submit([]() { return 10 + 20; });
    EXPECT_EQ(future.get(), 30);
}

// 测试带参数的任务
TEST(ThreadPoolTest, SubmitTaskWithArguments) {
    cppthreadflow::ThreadPool pool(2);
    auto future = pool.submit([](int a, const std::string& b) {
        return b + std::to_string(a);
    }, 42, "hello");
    EXPECT_EQ(future.get(), "hello42");
}

// 测试 void 返回类型的任务
TEST(ThreadPoolTest, SubmitVoidTask) {
    cppthreadflow::ThreadPool pool(1);
    std::atomic<bool> task_executed(false);
    auto future = pool.submit([&]() {
        task_executed.store(true);
    });
    future.get(); // 等待任务完成
    EXPECT_TRUE(task_executed);
}

// 测试任务中的异常传播
TEST(ThreadPoolTest, ExceptionPropagation) {
    cppthreadflow::ThreadPool pool(1);
    auto future = pool.submit([]() {
        throw std::runtime_error("This is a test exception");
    });
    // future.get() 应该重新抛出任务中的异常
    EXPECT_THROW(future.get(), std::runtime_error);
}

// 测试高并发提交任务
TEST(ThreadPoolTest, HighVolumeSubmission) {
    cppthreadflow::ThreadPool pool(8);
    const int num_tasks = 1000;
    std::vector<std::future<int>> futures;
    std::atomic<int> counter(0);

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i, &counter]() {
            counter++;
            return i * 2;
        }));
    }

    int sum = 0;
    for (int i = 0; i < num_tasks; ++i) {
     // 1. 调用 .get() 一次，并存储结果
     int result = futures[i].get();

     // 2. 使用存储的结果进行后续操作
     sum += result;
     EXPECT_EQ(result, i * 2);
    }

    EXPECT_EQ(counter, num_tasks);
    // 0*2 + 1*2 + ... + (n-1)*2 = 2 * (n-1)*n/2 = (n-1)*n
    EXPECT_EQ(sum, (num_tasks - 1) * num_tasks);
}

// 测试线程池的优雅关闭 (Graceful Shutdown)
TEST(ThreadPoolTest, GracefulShutdown) {
    std::atomic<int> tasks_completed(0);
    const int num_tasks = 5;

    // 将线程池置于一个独立的作用域内
    {
        cppthreadflow::ThreadPool pool(4);
        for (int i = 0; i < num_tasks; ++i) {
            pool.submit([&]() {
                // 模拟耗时任务
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                tasks_completed++;
            });
        }
        // 当离开这个作用域时，pool的析构函数会被调用
    }

    // 析构函数应该阻塞直到所有任务完成
    EXPECT_EQ(tasks_completed, num_tasks);
}
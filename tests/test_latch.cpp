#include <gtest/gtest.h>
#include "../src/ThreadLib/latch.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

// 1. 测试“完成信号”场景
// 主线程等待多个工作线程完成
TEST(LatchTest, CompletionSignal) {
    const int num_threads = 5;
    cppthreadflow::Latch latch(num_threads); // 初始化 Latch，计数为 5
    std::atomic<int> completed_tasks = 0;
    std::vector<std::thread> workers;

    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([&]() {
            // 模拟一些工作
            std::this_thread::sleep_for(10ms * (i % 2)); // 让线程完成时间错开
            completed_tasks++;
            latch.count_down(); // 工作完成，计数减 1
        });
    }

    // 主线程立即调用 wait()
    // 这将阻塞，直到所有 5 个线程都调用了 count_down()
    latch.wait();

    // 当 wait() 返回时，我们断言所有任务都已完成
    EXPECT_EQ(completed_tasks.load(), num_threads);

    // 清理所有线程
    for (auto& t : workers) {
        t.join();
    }
}

// 2. 测试“启动信号”场景
// 多个工作线程等待主线程的发令枪
TEST(LatchTest, StartSignal) {
    const int num_threads = 5;
    cppthreadflow::Latch start_gate(1); // 初始化 Latch，计数为 1
    std::atomic<int> threads_ready = 0;
    std::atomic<int> threads_released = 0;
    std::vector<std::thread> workers;

    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([&]() {
            // 线程准备就绪
            threads_ready++;
            // 在起跑门前等待
            start_gate.wait();
            // 被释放后，记录下来
            threads_released++;
        });
    }

    // 等待，确保所有线程都已启动并阻塞在 wait() 上
    while (threads_ready.load() != num_threads) {
        std::this_thread::sleep_for(10ms);
    }
    // 此时，不应有任何线程被释放
    EXPECT_EQ(threads_released.load(), 0);

    // 主线程发号施令（发令枪响）
    start_gate.count_down();

    // 给予足够的时间让所有线程被唤醒和调度
    std::this_thread::sleep_for(100ms);

    // 验证：所有线程都已被释放
    EXPECT_EQ(threads_released.load(), num_threads);

    for (auto& t : workers) {
        t.join();
    }
}

// 3. 测试 Latch 的一次性特性
// 一旦打开，永久打开
TEST(LatchTest, IsOneShot) {
    cppthreadflow::Latch latch(1);

    // 第一次 wait/count_down 周期
    std::thread t1([&]() {
        latch.wait();
    });

    // 给 t1 一点时间去阻塞
    std::this_thread::sleep_for(20ms);
    latch.count_down(); // 释放 t1
    t1.join();

    // 此时 Latch 的计数器为 0，门闩已永久打开

    // 第二次调用 wait()
    // 预期：立即返回，不会阻塞
    auto start_time = std::chrono::steady_clock::now();
    latch.wait();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // 断言执行时间非常短（例如，小于 5ms），证明没有发生阻塞
    EXPECT_LT(elapsed.count(), 5);

    // 第二次调用 count_down()
    // 预期：无任何效果，不会导致错误或崩溃
    latch.count_down();
    SUCCEED(); // 只要没崩溃就通过
}

// 4. 测试初始化为 0 的 Latch
TEST(LatchTest, InitializeWithZero) {
    cppthreadflow::Latch latch(0); // 初始化为 0

    // 预期：立即返回，不会阻塞
    auto start_time = std::chrono::steady_clock::now();
    latch.wait();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_LT(elapsed.count(), 5);
}
#include <gtest/gtest.h>
#include "../src/ThreadLib/semaphore.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <future>
#include <atomic>

using namespace std::chrono_literals;

// 1. 测试构造函数和 try_acquire
TEST(SemaphoreTest, InitialCountAndTryAcquire) {
    // 创建一个初始计数为 3 的信号量
    cppthreadflow::Semaphore sem(3);

    // 连续 3 次 try_acquire 应该都成功
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_TRUE(sem.try_acquire());

    // 第 4 次 try_acquire 应该失败，因为计数已为 0
    EXPECT_FALSE(sem.try_acquire());
}

// 2. 测试 acquire 的阻塞行为
TEST(SemaphoreTest, AcquireBlocksWhenCountIsZero) {
    // 初始计数为 0
    cppthreadflow::Semaphore sem(0);
    std::promise<void> thread_started_promise;
    std::promise<void> acquire_finished_promise;
    auto future = acquire_finished_promise.get_future();

    // 启动一个新线程
    std::thread t([&]() {
        thread_started_promise.set_value(); // 1. 通知主线程：我已启动
        sem.acquire();                      // 3. 将会阻塞在这里
        acquire_finished_promise.set_value(); // 5. 成功 acquire 后，通知主线程
    });

    thread_started_promise.get_future().wait(); // 2. 等待，确保新线程已运行

    // 检查 future 的状态，它不应该在短时间内变为 ready
    auto status = future.wait_for(100ms);
    // 4. 断言：线程仍在等待 (即 acquire 正在阻塞)
    EXPECT_EQ(status, std::future_status::timeout);

    // 释放信号量，应该会唤醒正在阻塞的线程 t
    sem.release();

    // 再次等待 future，这一次它应该很快变为 ready
    status = future.wait_for(1s); // 给足够的时间让线程被唤醒
    // 6. 断言：线程已被唤醒并完成了 acquire
    EXPECT_EQ(status, std::future_status::ready);

    t.join();
}

// 3. 测试 release 和 acquire 的配对
TEST(SemaphoreTest, ReleaseIncrementsCount) {
    cppthreadflow::Semaphore sem(0);

    // 释放 3 次
    sem.release();
    sem.release();
    sem.release();

    // 现在 acquire 3 次应该立即成功，而不会阻塞
    sem.acquire();
    sem.acquire();
    sem.acquire();

    // 此时计数又变为 0，try_acquire 应该失败
    EXPECT_FALSE(sem.try_acquire());
}

// 4. 多生产者、单消费者测试
TEST(SemaphoreTest, MultiProducerSingleConsumer) {
    cppthreadflow::Semaphore sem(0);
    const int num_producers = 10;
    std::vector<std::thread> producers;

    // 创建 10 个生产者线程，每个都释放一次信号量
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            sem.release();
        });
    }

    // 主线程（作为消费者）尝试获取 10 次
    for (int i = 0; i < num_producers; ++i) {
        sem.acquire(); // 这将阻塞，直到某个生产者 release
    }

    // 等待所有生产者线程完成
    for (auto& t : producers) {
        t.join();
    }

    // 所有资源都已被消耗，再次尝试获取应该失败
    EXPECT_FALSE(sem.try_acquire());
}

// 5. 单生产者、多消费者测试
TEST(SemaphoreTest, SingleProducerMultiConsumer) {
    cppthreadflow::Semaphore sem(0);
    const int num_consumers = 10;
    std::vector<std::thread> consumers;
    std::atomic<int> acquired_count = 0;

    // 创建 10 个消费者线程，每个都尝试获取一次
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            sem.acquire(); // 都会阻塞在这里
            acquired_count++;
        });
    }

    // 等待一小会儿，确保所有消费者线程都已启动并阻塞在 acquire() 上
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(acquired_count.load(), 0);

    // 主线程（作为生产者）释放 10 次资源
    for (int i = 0; i < num_consumers; ++i) {
        sem.release();
        // 每次 release 都应该唤醒一个消费者
        // 给一点点时间让线程被唤醒和调度
        std::this_thread::sleep_for(10ms);
    }

    // 等待所有消费者线程完成
    for (auto& t : consumers) {
        t.join();
    }

    // 验证：所有 10 个消费者都应该成功获取了信号量
    EXPECT_EQ(acquired_count.load(), num_consumers);
}
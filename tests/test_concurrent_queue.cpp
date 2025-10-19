#include <gtest/gtest.h>
#include "../src/ThreadLib/concurrent_queue.hpp"
#include <thread>
#include <vector>
#include <numeric>
#include <atomic>
#include <type_traits>
// 测试基本 Push 和 Pop 操作
TEST(ConcurrentQueueTest, BasicPushPop) {
    cppthreadflow::ConcurrentQueue<int> q;
    q.push(42);
    int val;
    ASSERT_TRUE(q.pop(val));
    EXPECT_EQ(val, 42);
}

// 测试队列停止行为
TEST(ConcurrentQueueTest, StopBehavior) {
    cppthreadflow::ConcurrentQueue<int> q;
    std::atomic<bool> thread_started(false);
    std::atomic<bool> pop_returned = false;

    std::thread t([&]() {
        int val;
        thread_started = true;
        // 这将会阻塞，因为队列是空的
        bool result = q.pop(val);
        pop_returned = true;
        EXPECT_FALSE(result); // pop 应该因为 stop 而返回 false
    });

    // 等待，确保线程已经启动并阻塞在 pop()
    while (!thread_started) {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(pop_returned); // 确认线程仍在阻塞

    q.stop(); // 停止队列
    t.join(); // join 线程

    EXPECT_TRUE(pop_returned); // 确认 pop 操作已返回
}

// 多生产者、多消费者压力测试
TEST(ConcurrentQueueTest, MPMCStressTest) {
    cppthreadflow::ConcurrentQueue<int> q;

     constexpr int num_producers = 4;
     constexpr int num_consumers = 4;
     constexpr int items_per_producer = 10000;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> items_produced(0);
    std::atomic<int> items_consumed (0);

    // 创建生产者线程
    producers.reserve(num_producers);
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < items_per_producer; ++j) {
                q.push(j);
                ++items_produced;
            }
        });
    }

    // 创建消费者线程
    consumers.reserve(num_consumers);
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            int val;
            // 只要队列没有被停止，就一直消费
            while (q.pop(val)) {
                ++items_consumed;
            }
        });
    }

    // 等待所有生产者完成
    for (auto& p : producers) {
        p.join();
    }

    // 等待一小段时间，确保消费者有机会处理完所有已入队的消息
    // 这是一个简单的同步方法，更复杂场景需要更精密的同步
    while (items_consumed < items_produced) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 停止队列，这将使消费者线程退出循环
    q.stop();

    // 等待所有消费者完成
    for (auto& c : consumers) {
        c.join();
    }

    // 最终断言：生产和消费的物品数量必须相等
    EXPECT_EQ(items_produced, num_producers * items_per_producer);
    EXPECT_EQ(items_consumed, items_produced.load());
}
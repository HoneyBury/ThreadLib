#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

#include "../src/ThreadLib/barrier.hpp"

using namespace std::chrono_literals;

// 1. 测试构造函数对 party_count 的约束
TEST(BarrierTest, Constructor) {
  // 构造函数参数必须是正数
  EXPECT_THROW(cppthreadflow::Barrier barrier(0), std::invalid_argument);
  EXPECT_THROW(cppthreadflow::Barrier barrier(-1), std::invalid_argument);

  // 构造一个合法的 barrier
  EXPECT_NO_THROW(cppthreadflow::Barrier barrier(1));
}

// 2. 测试基本的阻塞和释放行为
TEST(BarrierTest, BlocksAndReleases) {
  const int num_threads = 3;
  cppthreadflow::Barrier barrier(num_threads);

  std::atomic<int> threads_at_barrier = 0;
  std::atomic<int> threads_released = 0;
  std::vector<std::thread> workers;

  // 启动 N-1 = 2 个工作线程
  for (int i = 0; i < num_threads - 1; ++i) {
    workers.emplace_back([&]() {
      threads_at_barrier++;
      // 线程将阻塞在这里
      barrier.arrive_and_wait();
      threads_released++;
    });
  }

  // 等待，直到 2 个工作线程都已到达屏障
  while (threads_at_barrier.load() != num_threads - 1) {
    std::this_thread::sleep_for(10ms);
  }

  // 现在，2 个线程应该都在阻塞中。
  // 我们短暂睡眠，再次确认它们没有被释放。
  std::this_thread::sleep_for(50ms);
  EXPECT_EQ(threads_released.load(), 0);

  // 主线程作为第 N = 3 个线程到达
  threads_at_barrier++;
  barrier.arrive_and_wait();
  threads_released++;  // 主线程自己也算一个

  // 此时，屏障应该被触发，所有线程都应该被释放
  // 给予足够的时间让其他线程被唤醒和调度
  std::this_thread::sleep_for(50ms);

  // 验证：所有 3 个线程都已被释放
  EXPECT_EQ(threads_released.load(), num_threads);

  // 清理工作线程
  for (auto& t : workers) {
    t.join();
  }
}

// 3. 测试可重用性 (Cyclic / Deadlock Test)
TEST(BarrierTest, IsReusable) {
  const int num_threads = 5;
  const int num_cycles = 100;  // 我们将循环 100 次
  cppthreadflow::Barrier barrier(num_threads);
  std::atomic<int> completed_threads = 0;
  std::vector<std::thread> workers;

  // 启动 N = 5 个工作线程
  for (int i = 0; i < num_threads; ++i) {
    workers.emplace_back([&]() {
      // 每个线程都循环 100 次
      for (int cycle = 0; cycle < num_cycles; ++cycle) {
        // 在屏障处集合
        barrier.arrive_and_wait();
        // 模拟一些工作
        std::this_thread::yield();  // 出让 CPU，增加线程交错的可能性
      }
      // 成功完成所有循环
      completed_threads++;
    });
  }

  // 等待所有线程完成它们的所有循环
  for (auto& t : workers) {
    t.join();
  }

  // 验证：所有 5 个线程都成功退出了循环，没有发生死锁
  EXPECT_EQ(completed_threads.load(), num_threads);
}
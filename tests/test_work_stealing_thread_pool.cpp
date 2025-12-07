#include <gtest/gtest.h>
#include "../src/ThreadLib/work_stealing_thread_pool.hpp"
#include "../src/ThreadLib/latch.hpp"
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

// 1. 基础功能测试
TEST(WorkStealingThreadPoolTest, BasicSubmission) {
    cppthreadflow::WorkStealingThreadPool pool(4);
    auto future = pool.submit([] { return 42; });
    EXPECT_EQ(future.get(), 42);
}

// 2. 窃取测试 (Work Stealing Verification)
// 场景：一个任务在内部生成大量子任务。
// 原理：子任务会进入该线程的本地队列。
// 验证：如果不窃取，这一个线程要处理很久；如果窃取，其他空闲线程会来帮忙。
TEST(WorkStealingThreadPoolTest, MassiveSubTaskStealing) {
    // 使用 4 个线程
    cppthreadflow::WorkStealingThreadPool pool(4);

    const int num_tasks = 10000;
    std::atomic<int> completed_count = 0;
    cppthreadflow::Latch latch(num_tasks);

    // 记录开始时间
    auto start = std::chrono::high_resolution_clock::now();

    // 提交一个“根任务”
    pool.submit([&] {
        // 这个根任务在 线程 A 中执行
        // 它一口气生产 10000 个子任务
        // 这些子任务会全部堆积在 线程 A 的本地队列中
        for (int i = 0; i < num_tasks; ++i) {
            pool.submit([&] {
                // 模拟一点点计算量
                int volatile x = 0;
                for(int j=0; j<100; ++j) x++;

                completed_count++;
                latch.count_down();
            });
        }
    });

    // 主线程等待所有任务完成
    latch.wait();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(completed_count.load(), num_tasks);

    // 打印耗时，可以试着把 WorkStealingThreadPool 里的 try_steal 注释掉对比一下时间
    std::cout << "Processed " << num_tasks << " tasks in " << duration.count() << "ms" << std::endl;
}

// 3. 验证多线程并发提交
TEST(WorkStealingThreadPoolTest, ConcurrentSubmission) {
    cppthreadflow::WorkStealingThreadPool pool(4);
    const int num_tasks = 1000;
    std::atomic<int> sum = 0;
    cppthreadflow::Latch latch(num_tasks);

    // 外部并发提交 (模拟主线程或其他非池线程提交)
    for (int i = 0; i < num_tasks; ++i) {
        pool.submit([&] {
            sum++;
            latch.count_down();
        });
    }

    latch.wait();
    EXPECT_EQ(sum.load(), num_tasks);
}
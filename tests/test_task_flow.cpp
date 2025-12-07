#include <gtest/gtest.h>
#include "../src/ThreadLib/task_flow.hpp"
#include "../src/ThreadLib/work_stealing_thread_pool.hpp"
#include <atomic>
#include <vector>

TEST(TaskFlowTest, SimpleDependency) {
    cppthreadflow::WorkStealingThreadPool pool(4);
    cppthreadflow::TaskFlow flow;

    int x = 0;

    // A -> B
    auto A = flow.emplace([&] { x = 1; });
    auto B = flow.emplace([&] { x += 2; });

    flow.precede(A, B); // B 依赖 A

    flow.run(pool).get(); // 等待完成

    EXPECT_EQ(x, 3);
}

TEST(TaskFlowTest, ComplexDiamondShape) {
    // 菱形依赖：
    //   A
    //  / \
    // B   C
    //  \ /
    //   D

    cppthreadflow::WorkStealingThreadPool pool(4);
    cppthreadflow::TaskFlow flow;

    std::atomic<int> counter = 0;
    std::atomic<int> b_c_sum = 0;

    auto A = flow.emplace([&] {
        counter = 1;
    });

    auto B = flow.emplace([&] {
        // 只有 A 完成了，这里才能读到 1
        int val = counter.fetch_add(1);
        // 期望 val 是 1 (A的结果) 或 2 (C已经执行了)
    });

    auto C = flow.emplace([&] {
        counter.fetch_add(1);
    });

    auto D = flow.emplace([&] {
        // B 和 C 都完成了，A 也完成了
        // 初始0 + A(1) + B(1) + C(1) = 3
        if (counter.load() == 3) {
            counter = 100; // 标记成功
        }
    });

    flow.precede(A, B);
    flow.precede(A, C);
    flow.precede(B, D);
    flow.precede(C, D);

    flow.run(pool).get();

    EXPECT_EQ(counter.load(), 100);
}

TEST(TaskFlowTest, ParallelExecution) {
    // 验证 B 和 C 是并行执行的（理论上）
    // A -> [B, C, D] -> E
    cppthreadflow::WorkStealingThreadPool pool(4);
    cppthreadflow::TaskFlow flow;

    std::atomic<int> active_tasks = 0;
    std::atomic<bool> had_parallelism = false;

    auto A = flow.emplace([]{});
    auto E = flow.emplace([]{});

    auto make_parallel_task = [&](int ms) {
        return flow.emplace([&, ms] {
            active_tasks++;
            if (active_tasks > 1) had_parallelism = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            active_tasks--;
        });
    };

    auto B = make_parallel_task(10);
    auto C = make_parallel_task(10);
    auto D = make_parallel_task(10);

    flow.precede(A, B); flow.precede(B, E);
    flow.precede(A, C); flow.precede(C, E);
    flow.precede(A, D); flow.precede(D, E);

    flow.run(pool).get();

    // 注意：并行性不是绝对保证的（取决于调度），但在4线程池+3个sleep任务中，概率极高
    EXPECT_TRUE(had_parallelism);
}
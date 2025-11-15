#include <benchmark/benchmark.h>
#include "ThreadLib/thread_pool.hpp"
#include "ThreadLib/latch.hpp" // 我們用 Latch 來等待任務完成
#include <atomic>
#include <vector>
static void BM_SingleThread_TaskExecution(benchmark::State& state) {
    const int num_tasks = state.range(0);
    std::atomic<int> counter(0);
    auto task = [&counter]() {
        benchmark::DoNotOptimize(counter++);
    };

    for (auto _ : state) {
        counter = 0;
        for (int i = 0; i < num_tasks; ++i) {
            task();
        }
    }
    state.SetItemsProcessed(state.iterations() * num_tasks);
}

// 2. 測試我們的 ThreadPool
static void BM_ThreadPool_TaskExecution(benchmark::State& state) {
    // 關鍵：使用 static pool。這可能會導致死鎖，但在你的機器上它碰巧運行成功了
    static cppthreadflow::ThreadPool pool(8);
    const int num_tasks = state.range(0);
    std::atomic<int> counter(0);
    auto task = [&counter]() {
        benchmark::DoNotOptimize(counter++);
    };

    for (auto _ : state) {
        cppthreadflow::Latch latch(num_tasks);
        counter = 0;

        for (int i = 0; i < num_tasks; ++i) {
            pool.submit([&]() {
                task();
                latch.count_down();
            });
        }
        latch.wait();
    }
    state.SetItemsProcessed(state.iterations() * num_tasks);
}

// 註冊測試
BENCHMARK(BM_SingleThread_TaskExecution)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime();

BENCHMARK(BM_ThreadPool_TaskExecution)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime();
#include <benchmark/benchmark.h>
#include "ThreadLib/concurrent_hash_map.hpp"
#include <unordered_map>
#include <mutex>
#include <string>

template<typename K, typename V>
class SingleLockMap {
public:
    void insert(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_[key] = value;
    }
    bool find(const K& key, V& value_out) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            value_out = it->second;
            return true;
        }
        return false;
    }
private:
    std::mutex mutex_;
    std::unordered_map<K, V> map_;
};

// --- 測試案例 (使用 static 變量) ---

// 1. 測試我們的 ConcurrentHashMap
static void BM_ConcurrentHashMap_Writes(benchmark::State& state) {
    // 關鍵：static 變量在所有測試運行之間共享
    static cppthreadflow::ConcurrentHashMap<int, int> map(64);
    int key = state.thread_index() * state.range(0) + state.iterations();

    for (auto _ : state) {
        map.insert(key, key);
        key++; // 確保 key 是唯一的
    }
}

// 2. 測試基線 SingleLockMap
static void BM_SingleLockMap_Writes(benchmark::State& state) {
    // 關鍵：static 變量在所有測試運行之間共享
    static SingleLockMap<int, int> map;
    int key = state.thread_index() * state.range(0) + state.iterations();

    for (auto _ : state) {
        map.insert(key, key);
        key++;
    }
}

// 註冊測試
BENCHMARK(BM_ConcurrentHashMap_Writes)
    ->Arg(10000) // 每個線程操作的次數
    ->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Threads(16)
    ->UseRealTime(); // **關鍵：並發測試必須使用 RealTime**

BENCHMARK(BM_SingleLockMap_Writes)
    ->Arg(10000)
    ->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Threads(16)
    ->UseRealTime();
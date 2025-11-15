#include <gtest/gtest.h>
#include <thread>
#include "../src/ThreadLib/concurrent_hash_map.hpp"
#include <vector>
#include <string>
#include <atomic>

// 1. 測試基本的單線程操作 (Insert, Find)
TEST(ConcurrentHashMapTest, BasicInsertAndFind) {
    cppthreadflow::ConcurrentHashMap<int, std::string> map(4); // 4 個分片

    map.insert(1, "one");
    map.insert(5, "five"); // 假設 1 和 5 哈希到不同的分片

    std::string value;
    ASSERT_TRUE(map.find(1, value));
    EXPECT_EQ(value, "one");

    ASSERT_TRUE(map.find(5, value));
    EXPECT_EQ(value, "five");

    ASSERT_FALSE(map.find(99, value));
}

// 2. 測試單線程更新 (Insert-Or-Update)
TEST(ConcurrentHashMapTest, InsertOrUpdate) {
    cppthreadflow::ConcurrentHashMap<int, int> map(2);

    map.insert(10, 100);
    int value;
    ASSERT_TRUE(map.find(10, value));
    EXPECT_EQ(value, 100);

    // 插入相同的鍵，值應該被覆蓋
    map.insert(10, 200);
    ASSERT_TRUE(map.find(10, value));
    EXPECT_EQ(value, 200);
}

// 3. 測試單線程刪除 (Erase)
TEST(ConcurrentHashMapTest, Erase) {
    cppthreadflow::ConcurrentHashMap<int, int> map(2);
    map.insert(1, 1);

    ASSERT_TRUE(map.erase(1)); // 成功刪除
    ASSERT_FALSE(map.erase(1)); // 再次刪除應失敗

    int value;
    ASSERT_FALSE(map.find(1, value)); // 驗證已找不到
    ASSERT_FALSE(map.erase(99));      // 刪除不存在的鍵
}

// 4. 測試 Clear 和 Size
TEST(ConcurrentHashMapTest, ClearAndSize) {
    cppthreadflow::ConcurrentHashMap<int, int> map(8);

    // 插入 100 個元素
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i);
    }
    EXPECT_EQ(map.size(), 100);

    map.clear();
    EXPECT_EQ(map.size(), 0);

    int value;
    ASSERT_FALSE(map.find(50, value)); // 驗證已清空
}

// 5. 測試移動語義 (Move Semantics)
TEST(ConcurrentHashMapTest, MoveSemantics) {
    cppthreadflow::ConcurrentHashMap<int, std::unique_ptr<int>> map(4);

    auto ptr = std::make_unique<int>(42);
    map.insert(1, std::move(ptr)); // ptr 應該被移入 map

    // 驗證：原始 ptr 變為空
    EXPECT_EQ(ptr, nullptr);

    std::unique_ptr<int> out_ptr;
    // find() 是拷貝，對於 move-only 類型無法工作
    // 我們的 find() 實現是拷貝，所以這個測試需要修改 find 實現
    // 暫時我們先測試 string 的 move

    cppthreadflow::ConcurrentHashMap<int, std::string> str_map(4);
    std::string s = "hello world";
    str_map.insert(1, std::move(s));

    EXPECT_TRUE(s.empty()); // 驗證 s 已被移走
    std::string val;
    ASSERT_TRUE(str_map.find(1, val));
    EXPECT_EQ(val, "hello world");
}

// 6. 並發插入測試 (高並發壓力測試)
TEST(ConcurrentHashMapTest, ConcurrentInsert) {
    const int num_threads = 8;
    const int items_per_thread = 10000;
    // 創建一個 16 分片的 map，減少不同線程間的鎖競爭
    cppthreadflow::ConcurrentHashMap<int, int> map(16);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&map, i, items_per_thread]() {
            for (int j = 0; j < items_per_thread; ++j) {
                // 每個線程插入不同的鍵
                int key = i * items_per_thread + j;
                map.insert(key, key + 1);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 驗證：所有元素都已成功插入
    EXPECT_EQ(map.size(), num_threads * items_per_thread);
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < items_per_thread; ++j) {
            int key = i * items_per_thread + j;
            int value;
            ASSERT_TRUE(map.find(key, value));
            EXPECT_EQ(value, key + 1);
        }
    }
}

// 7. 並發讀寫混合測試 (讀/寫/刪)
TEST(ConcurrentHashMapTest, ConcurrentMixedWorkload) {
    const int num_threads = 8;
    const int ops_per_thread = 10000;
    // 鍵空間
    const int key_range = 1000;
    cppthreadflow::ConcurrentHashMap<int, int> map(32); // 更多分片
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&map, i, ops_per_thread, key_range]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                int key = (j + i) % key_range;
                int op_type = j % 3;

                switch (op_type) {
                    case 0: // 插入/更新
                        map.insert(key, i);
                        break;
                    case 1: // 查找
                        int value;
                        map.find(key, value);
                        break;
                    case 2: // 刪除
                        map.erase(key);
                        break;
                }
            }
        });
    }

    // 等待所有線程完成
    for (auto& t : threads) {
        t.join();
    }

    // 在這種混亂測試中，我們很難驗證最終狀態。
    // 這個測試的主要目的是：
    // 1. 檢測是否存在死鎖 (Deadlock) -> 如果 join() 成功，就沒有死鎖。
    // 2. 檢測是否存在內存訪問沖突 (Data Race) -> 需要用 ThreadSanitizer 運行。
    // 3. 檢測是否崩潰 (Crash) -> 如果程序沒崩潰，就通過。
    SUCCEED() << "Chaos test completed without deadlock or crash.";
}
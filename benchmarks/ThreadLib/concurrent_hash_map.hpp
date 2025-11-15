#pragma once

#include <functional>  // for std::hash
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>  // for std::pair
#include <vector>
namespace cppthreadflow {

/**
 * @brief 一个高性能的、基于分片锁的线程安全哈希表。
 *
 * @tparam Key 键类型。
 * @tparam Value 值类型。
 * @tparam Hash 哈希函数，默认为 std::hash<Key>。
 * @tparam KeyEqual 键比较函数，默认为 std::equal_to<Key>。
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key> >
class ConcurrentHashMap {
 private:
  /**
   * @brief 分片 (Shard) 结构体。
   * 每个分片包含一个独立的哈希表和一个独立的互斥锁。
   * 注意：mutex 必须是 mutable，以便在 const 成员函数（如 find）中被锁定。
   */
  struct Shard {
    mutable std::mutex mutex_;
    std::unordered_map<Key, Value, Hash, KeyEqual> map_;
  };

 public:
  /**
   * @brief 构造一个并发哈希表。
   * @param concurrency_level 预期的并发级别，用于确定分片的数量。
   * 默认为硬件并发线程数。
   */
  explicit ConcurrentHashMap(
      size_t concurrency_level = std::thread::hardware_concurrency())
      : num_shards_(concurrency_level) {
    if (num_shards_ == 0) {
      num_shards_ = 1;
    }
    // 初始化分片，创建 num_shards_ 个 Shard 实例
    shards_ = std::make_unique<Shard[]>(num_shards_);
  }

  // 禁止拷贝和移动（因为内部有锁和唯一指针）
  ConcurrentHashMap(const ConcurrentHashMap&) = delete;
  ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;

  /**
   * @brief 插入一个键值对。
   * @param key 键。
   * @param value 值。
   */
  void insert(const Key& key, const Value& value) {
    Shard& shard = get_shard(key);
    std::unique_lock<std::mutex> lock(shard.mutex_);
    shard.map_[key] = value;  // 使用 operator[] 实现插入或更新
  }

  /**
   * @brief 插入一个键值对（移动语义）。
   * @param key 键。
   * @param value 值（将被移动）。
   */
  void insert(const Key& key, Value&& value) {
    Shard& shard = get_shard(key);
    std::unique_lock<std::mutex> lock(shard.mutex_);
    shard.map_[key] = std::move(value);
  }

  /**
   * @brief 查找一个键。
   * @param key 要查找的键。
   * @param value_out [输出参数] 如果找到，值将被拷贝到这里。
   * @return 如果找到键，返回 true，否则返回 false。
   */
  bool find(const Key& key, Value& value_out) const {
    const Shard& shard = get_shard(key);
    std::unique_lock<std::mutex> lock(shard.mutex_);  // 锁是 mutable 的

    auto it = shard.map_.find(key);
    if (it != shard.map_.end()) {
      value_out = it->second;  // 拷贝值
      return true;
    }
    return false;
  }

  /**
   * @brief 移除一个键。
   * @param key 要移除的键。
   * @return 如果成功移除，返回 true，否则返回 false。
   */
  bool erase(const Key& key) {
    Shard& shard = get_shard(key);
    std::unique_lock<std::mutex> lock(shard.mutex_);

    // std::unordered_map::erase(key) 返回移除的元素数量
    return shard.map_.erase(key) > 0;
  }

  /**
   * @brief 清空整个哈希表。
   * 注意：这是一个开销很大的操作，它会逐个锁定所有分片。
   */
  void clear() {
    for (size_t i = 0; i < num_shards_; ++i) {
      std::unique_lock<std::mutex> lock(shards_[i].mutex_);
      shards_[i].map_.clear();
    }
  }

  /**
   * @brief 获取哈希表中的元素总数。
   * 注意：这是一个估算值，因为在计算时其他线程可能正在修改。
   * 这是一个相对较慢的操作，因为它需要遍历所有分片。
   * @return 元素总数。
   */
  size_t size() const {
    size_t total_size = 0;
    for (size_t i = 0; i < num_shards_; ++i) {
      std::unique_lock<std::mutex> lock(shards_[i].mutex_);
      total_size += shards_[i].map_.size();
    }
    return total_size;
  }

 private:
  /**
   * @brief 根据键的哈希值获取对应的分片。
   * @param key 键。
   * @return 对应的分片引用。
   */
  Shard& get_shard(const Key& key) const {
    // 1. 计算键的哈希值
    size_t hash_val = hasher_(key);
    // 2. 通过取模找到分片索引
    size_t shard_index = hash_val % num_shards_;
    return shards_[shard_index];
  }

  Hash hasher_;
  size_t num_shards_;
  // 使用 unique_ptr<Shard[]> 来持有分片数组，确保正确的内存管理
  std::unique_ptr<Shard[]> shards_;
};

}  // namespace cppthreadflow
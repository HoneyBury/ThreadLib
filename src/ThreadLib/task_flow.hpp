#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cppthreadflow {

class WorkStealingThreadPool;  // 前向声明

class TaskFlow {
 public:
  struct TaskNode {
    std::function<void()> work;
    std::vector<TaskNode*> successors;  // 后继节点列表
    int in_degree = 0;                  // 静态入度（构建时确定）

    template <typename F>
    explicit TaskNode(F&& f) : work(std::forward<F>(f)) {}
  };

  // TaskHandle 本质上是指向 TaskNode 的指针
  using TaskHandle = TaskNode*;

  TaskFlow() = default;
  ~TaskFlow() = default;

  TaskFlow(const TaskFlow&) = delete;
  TaskFlow& operator=(const TaskFlow&) = delete;
  TaskFlow(TaskFlow&&) = default;
  TaskFlow& operator=(TaskFlow&&) = default;

  template <typename F>
  TaskHandle emplace(F&& func) {
    auto node = std::make_unique<TaskNode>(std::forward<F>(func));
    TaskHandle handle = node.get();
    nodes_.push_back(std::move(node));
    return handle;
  }

  void precede(TaskHandle precede, TaskHandle succeed);

  std::future<void> run(WorkStealingThreadPool& pool);

 private:
  // 雖然 TaskNode 定義是公開的，但存儲它們的容器依然是私有的
  std::vector<std::unique_ptr<TaskNode>> nodes_;
};

}  // namespace cppthreadflow
#include "task_flow.hpp"

#include "work_stealing_thread_pool.hpp"

namespace cppthreadflow {

// 内部运行时状态
struct GraphRuntime {
    std::promise<void> promise;
    std::atomic<int> tasks_remaining;

    // 动态入度表：索引对应 nodes_ref 中的下标
    std::vector<std::atomic<int>> dynamic_indegrees;

    // 映射：TaskNode* -> Index
    std::unordered_map<TaskFlow::TaskNode*, size_t> node_map;

    // 為了避免頻繁解引用 unique_ptr，我們緩存裸指針
    std::vector<TaskFlow::TaskNode*> nodes_ref;

    GraphRuntime(size_t size) : tasks_remaining(static_cast<int>(size)), dynamic_indegrees(size) {}
};

// 前向声明辅助函数
// 注意：我们在匿名命名空间中定义它，以避免符号污染
namespace {
    void schedule_node(size_t node_idx, std::shared_ptr<GraphRuntime> rt, WorkStealingThreadPool& pool);
}

void TaskFlow::precede(TaskHandle pre, TaskHandle suc) {
    if (pre && suc) {
        pre->successors.push_back(suc);
        suc->in_degree++;
    }
}

std::future<void> TaskFlow::run(WorkStealingThreadPool& pool) {
    if (nodes_.empty()) {
        std::promise<void> p;
        p.set_value();
        return p.get_future();
    }

    auto runtime = std::make_shared<GraphRuntime>(nodes_.size());
    auto future = runtime->promise.get_future();

    // 初始化映射和入度
    for (size_t i = 0; i < nodes_.size(); ++i) {
        runtime->node_map[nodes_[i].get()] = i;
        runtime->nodes_ref.push_back(nodes_[i].get());
        runtime->dynamic_indegrees[i].store(nodes_[i]->in_degree);
    }

    // 查找所有入度为 0 的节点（入口点）
    std::vector<size_t> entries;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i]->in_degree == 0) {
            entries.push_back(i);
        }
    }

    // 启动入口节点
    for (size_t idx : entries) {
        schedule_node(idx, runtime, pool);
    }

    return future;
}

// 辅助函数实现
namespace {
    void schedule_node(size_t node_idx, std::shared_ptr<GraphRuntime> rt, WorkStealingThreadPool& pool) {
        // 使用 pool.submit 提交任务
        pool.submit([node_idx, rt, &pool]() {
            auto* node = rt->nodes_ref[node_idx];

            // 1. 执行用户任务
            if (node->work) {
                node->work();
            }

            // 2. 触发后继节点
            for (auto* successor : node->successors) {
                // 找到后继节点的索引
                size_t succ_idx = rt->node_map[successor];

                // 原子递减入度。如果 fetch_sub 返回 1，说明减完后变成了 0
                if (rt->dynamic_indegrees[succ_idx].fetch_sub(1) == 1) {
                    // 递归调度（实际上是向线程池提交新任务，不会爆栈）
                    schedule_node(succ_idx, rt, pool);
                }
            }

            // 3. 检查是否所有任务都完成了
            // 如果 fetch_sub 返回 1，说明这是最后一个任务
            if (rt->tasks_remaining.fetch_sub(1) == 1) {
                rt->promise.set_value();
            }
        });
    }
}

} // namespace cppthreadflow
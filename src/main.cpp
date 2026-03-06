#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include "lockFreeQueue.hpp"  // 假设你的头文件名为 lock_free_queue.hpp

void test_single_thread() {
    LockFreeQueue<int> q;

    // 测试空队列 pop
    assert(!q.pop());

    // 测试 push 和 pop
    q.push(42);
    auto val = q.pop();
    // printf("Popped value: %d\n", *val);
    assert(val && *val == 42);

    // 再次 pop 应为空
    assert(!q.pop());

    std::cout << "Single-thread test passed.\n";
}

void test_multi_thread() {
    const int num_threads = 4;
    const int items_per_thread = 1000;
    LockFreeQueue<int> q;

    // 启动多个生产者线程
    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
        producers.emplace_back([&q, i, items_per_thread]() {
            for (int j = 0; j < items_per_thread; ++j) {
                q.push(i * items_per_thread + j);
            }
        });
    }

    // 启动消费者线程（也可以在主线程消费）
    std::vector<int> results;
    std::vector<std::thread> consumers;
    std::mutex result_mutex;

    // 使用一个消费者线程（也可多个，但需注意竞争）
    std::thread consumer([&q, &results, &result_mutex, total = num_threads * items_per_thread]() {
        while (results.size() < static_cast<size_t>(total)) {
            auto val = q.pop();
            if (val) {
                std::lock_guard<std::mutex> lock(result_mutex);
                results.push_back(*val);
            }
            // 可加短暂休眠避免忙等待（非必须）
            // std::this_thread::yield();
        }
    });

    for (auto& t : producers) t.join();
    consumer.join();

    // 验证数量
    assert(results.size() == num_threads * items_per_thread);

    // 验证所有值是否唯一（因为 push 的值是唯一的）
    std::vector<bool> seen(num_threads * items_per_thread, false);
    for (int v : results) {
        assert(v >= 0 && v < num_threads * items_per_thread);
        assert(!seen[v]);  // 不应重复
        seen[v] = true;
    }

    std::cout << "Multi-thread test passed. Total items: " << results.size() << "\n";
}

int main() {
    test_single_thread();
    test_multi_thread();
    std::cout << "All tests passed!\n";
    return 0;
}
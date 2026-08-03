/*  datastruct 模块性能测试
    测试内容：
    - CircularQueue 入队/出队吞吐量
    - 容量满时的覆盖行为开销
    - 与 std::queue 的对比
*/
#pragma once
#ifndef BENCH_DATASTRUCT_HPP
#define BENCH_DATASTRUCT_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <queue>

using namespace meido;

namespace _meidodsbench
{

inline void check()
{
    BENCH_MODULE("datastruct 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. CircularQueue 入队（容量足够，不触发覆盖） ----
    // 设计意图：验证 CircularQueue 的基础入队性能。
    // 内部操作：tail_ 处写入元素，tail_ 循环递增，count_ 递增。
    // 不触发覆盖时只需 O(1) 赋值 + 取模运算。
    {
        const int64_t N = 300000000;
        const int64_t N_scaled = N;
        ds::CircularQueue<int> q(N_scaled + 100);
        BENCH_N("CircularQueue<int> enqueue (不触发覆盖)", {
            q.enqueue(42);
        });
        volatile auto sink = q.empty();
        (void)sink;
    }
    BENCH_SEP();

    // ---- 2. CircularQueue 入队+出队（满吞吐交替） ----
    // 设计意图：验证生产者-消费者模式的真实吞吐。
    // 每次迭代一次 enqueue + 一次 tryDequeue，验证 head_/tail_ 指针管理。
    {
        ds::CircularQueue<int> q(64);
        for (int i = 0; i < 64; i++) q.enqueue(i);

        int dst = 0;
        BENCH_N("CircularQueue<int> enqueue+dequeue (容量64)", {
            q.enqueue(42);
            q.tryDequeue(dst);
        });
        volatile auto sink = dst;
        (void)sink;
    }
    BENCH_SEP();

    // ---- 3. CircularQueue 满容量时覆盖旧元素 ----
    // 设计意图：容量满时 enqueue 自动覆盖 head_ 处元素。
    // head_/tail_ 相遇时 head_ 前移，逻辑比未满时多一个分支。
    {
        ds::CircularQueue<int> q(16);
        for (int i = 0; i < 16; i++) q.enqueue(i);
        BENCH_N("CircularQueue<int> enqueue 覆盖旧元素 (容量16)", {
            q.enqueue(42);
        });
        volatile auto sink = q.empty();
        (void)sink;
    }
    BENCH_SEP();

    // ---- 4. CircularQueue 空队列出队 ----
    // 设计意图：空队列时 tryDequeue 立即返回 false，不操作 dst。
    // 验证"失败路径"的性能，预期接近零开销。
    {
        ds::CircularQueue<int> q(16);
        int dst = 0;
        BENCH_N("CircularQueue<int> tryDequeue 空队列", {
            q.tryDequeue(dst);
        });
        volatile auto sink = dst;
        (void)sink;
    }
    BENCH_SEP();

    // ---- 5. 对比：std::queue 入队+出队 ----
    // 设计意图：基准参考，std::queue 底层为 deque，每次 push 可能涉及
    // 内存分配，远慢于 CircularQueue 的固定缓冲区
    {
        std::queue<int> q;
        BENCH_N("std::queue<int> push+pop (参考)", {
            q.push(42);
            q.pop();
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidodsbench

#endif // BENCH_DATASTRUCT_HPP

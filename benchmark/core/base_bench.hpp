/*  base 模块性能测试
    测试内容：
    - ScopeGuard / makeScopeGuard 构造与析构开销
    - MEIDO_ASSERT 通过时的开销
    - getVersion() / isBigEndian() 常量表达式开销（编译期求值，运行时为0）
*/
#pragma once
#ifndef BENCH_BASE_HPP
#define BENCH_BASE_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"

using namespace meido;

namespace _meidobasebench
{

inline void check()
{
    BENCH_MODULE("base 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. makeScopeGuard + 立即析构（lambda） ----
    // 设计意图：验证 ScopeGuard 作为 RAII 守卫的基础构造+析构开销。
    // N=100B → 预计 ~24s → ~2400 samples（配合校准共 ~4000+）
    {
        volatile int sink = 0;
        BENCH_N_CONSTEXPR("ScopeGuard(lambda) 构造+析构", {
            auto guard = base::makeScopeGuard([&sink]() { sink = 1; });
            (void)guard;
        });
        (void)sink;
    }
    BENCH_SEP();

    // ---- 2. makeScopeGuard + dismiss ----
    // 设计意图：dismiss() 将 valid_ 置 false，析构时跳过回调。
    // 编译器可证明 lambda 永不执行，从而消除整个 ScopeGuard 及循环。
    // 即使 N=50B，预期仍为 0ms — 这是编译器优化的结果，而非测试设计问题。
    {
        volatile int sink = 0;
        BENCH_N_CONSTEXPR("ScopeGuard(lambda) 构造+dismiss", {
            auto guard = base::makeScopeGuard([&sink]() { sink = 1; });
            guard.dismiss();
        });
        (void)sink;
    }
    BENCH_SEP();

    // ---- 3. makeScopeGuard + reset ----
    // 设计意图：reset() 执行 lambda(counter++) 后置 valid_=false，
    // 析构时 reset() 因 valid_=false 为空操作。
    // 编译器可能将 N 次递增合并为 counter = N，导致循环被消除。
    // N=50B 仍显示 0ms 则证明合并确实发生了。
    {
        int counter = 0;
        BENCH_N_CONSTEXPR("ScopeGuard(lambda) 构造+reset", {
            auto guard = base::makeScopeGuard([&counter]() { counter++; });
            guard.reset();
        });
        volatile int sink = counter;
        (void)sink;
    }
    BENCH_SEP();

    // ---- 4. MEIDO_ASSERT 通过时的开销 ----
    // 设计意图：x=42 是编译期常量，x==42 恒为 true。
    // 编译器可以完全消除整个断言宏。
    // N=50B 仍显示 0ms 则证明宏在通过场景下真正零成本。
    {
        int x = 42;
        BENCH_N_CONSTEXPR("MEIDO_ASSERT(通过) 开销", {
            MEIDO_ASSERT(x == 42);
        });
        (void)x;
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidobasebench

#endif // BENCH_BASE_HPP

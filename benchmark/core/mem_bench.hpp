/*  mem 模块性能测试
    测试内容：
    - ObjectPool construct/destroy 吞吐量
    - 与 new/delete 的对比
    - 池满时的 nullptr 返回开销
*/
#pragma once
#ifndef BENCH_MEM_HPP
#define BENCH_MEM_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <vector>

using namespace meido;

namespace _meidomembench
{

// 一个简单的测试类型
struct _TestObj
{
    int a;
    double b;
    char c;
    _TestObj() : a(0), b(0.0), c(0) {}
    _TestObj(int a_, double b_, char c_) : a(a_), b(b_), c(c_) {}
};

// noinline 包装函数，阻止编译器通过 escape analysis 将 new/delete 优化为栈分配。
// 这是测量真实堆分配开销的前提。
#if defined(__GNUC__)
__attribute__((noinline))
static _TestObj* _heapAlloc() { return new _TestObj(1, 2.0, 'x'); }
__attribute__((noinline))
static void _heapFree(_TestObj* p) { delete p; }
#else
__declspec(noinline)
static _TestObj* _heapAlloc() { return new _TestObj(1, 2.0, 'x'); }
__declspec(noinline)
static void _heapFree(_TestObj* p) { delete p; }
#endif

inline void check()
{
    BENCH_MODULE("mem 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. ObjectPool construct+destroy 配对（大池） ----
    // 设计意图：验证大容量池下"分配+释放"配对吞吐（free_list_ 指针跳跃，cache 不友好）。
    // 每 op 构造后立即归还，池不会满，construct 恒返回有效指针。
    // 规模默认 1000 万（≈240MB），可用 --mem-n 覆盖，避免小内存机器 OOM。
    {
        const int64_t N = bench::config().mem_n;
        mem::ObjectPool<_TestObj> pool(N);
        std::cout << "[INFO] mem 规模: " << N << " 对象 (≈"
                  << N * sizeof(_TestObj) / 1024 / 1024 << " MB)" << std::endl;

        BENCH_N("ObjectPool construct+destroy 配对 (大池)", {
            auto* p = pool.construct(1, 2.0, 'x');
            if (p) pool.destroy(p);
        });
    }
    BENCH_SEP();

    // ---- 2. ObjectPool 循环利用（固定容量64） ----
    // 设计意图：固定小容量循环分配/释放，模拟高频短生命周期对象。
    // 每次 construct 从 free_list_ 取一个，destroy 归还。
    // 由于不会满，free_list_ 始终非空。
    {
        mem::ObjectPool<_TestObj> pool(64);

        BENCH_N("ObjectPool construct+destroy 循环64槽", {
            auto* p = pool.construct(1, 2.0, 'x');
            if (p) pool.destroy(p);
        });
    }
    BENCH_SEP();

    // ---- 3. ObjectPool 池满返回 nullptr ----
    // 设计意图：池满时 construct 立即返回 nullptr。
    // 验证"失败路径"开销：mutex 锁定 + free_list_ 判空。
    {
        mem::ObjectPool<_TestObj> pool(64);
        for (int i = 0; i < 64; i++) pool.construct();

        BENCH_N("ObjectPool 池满 → nullptr 返回", {
            volatile auto* p = pool.construct();
            (void)p;
        });
    }
    BENCH_SEP();

    // ---- 4. 对比：raw new/delete ----
    // 设计意图：标准堆分配作为基准参考。
    // 使用 noinline 包装函数，阻止编译器通过 escape analysis 将堆分配优化为栈分配。
    {
        BENCH_N("raw new/delete (参考, noinline)", {
            auto* p = _heapAlloc();
            volatile auto sink = p->a;
            (void)sink;
            _heapFree(p);
        });
    }
    BENCH_SEP();

    // ---- 5. 对比：std::vector emplace_back（预分配） ----
    // 设计意图：vector 预分配后 emplace_back 仅 placement new，不触发 realloc。
    {
        std::vector<_TestObj> vec;
        vec.reserve(64);
        BENCH_N("vector<_TestObj> emplace_back (预分配)", {
            vec.emplace_back(1, 2.0, 'x');
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidomembench

#endif // BENCH_MEM_HPP

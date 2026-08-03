/*  time 模块性能测试
    测试内容：
    - nowSystem / nowSteady 获取时间开销
    - TimePoint 构造、转换、计算
    - MeanTimeCounter 统计开销
    - TimeCounterScope RAII 计时开销
    - sleep 函数（短时间）
*/
#pragma once
#ifndef BENCH_TIME_HPP
#define BENCH_TIME_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <string>
#include <sstream>
#if defined(_MSC_VER)
#include <windows.h>
#endif

using namespace meido;

namespace _meidotimebench
{

inline void check()
{
    BENCH_MODULE("time 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. nowSystem / nowSteady ----
    // 设计意图：获取系统时间的原始开销。底层为 clock_gettime(CLOCK_REALTIME/CLOCK_MONOTONIC)，
    // 在 Linux 上通过 vDSO 实现，不陷入内核。
    {
        BENCH_N("nowSystem() 获取系统时间", {
            volatile auto tp = time::nowSystem();
            (void)tp;
        });
        BENCH_N("nowSteady() 获取稳态时间", {
            volatile auto tp = time::nowSteady();
            (void)tp;
        });
    }
    BENCH_SEP();

    // ---- 2. 原始 clock_gettime 对比（仅 Linux） ----
    // 设计意图：直接调用系统调用的开销，作为 TimePoint 的基准参考
#if !defined(_MSC_VER)
    {
        struct timespec ts;
        BENCH_N("raw clock_gettime(CLOCK_MONOTONIC)", {
            clock_gettime(CLOCK_MONOTONIC, &ts);
        });
    }
#else
    {
        BENCH_N("raw QueryPerformanceCounter (Windows)", {
            LARGE_INTEGER cnt;
            QueryPerformanceCounter(&cnt);
        });
    }
#endif
    BENCH_SEP();

    // ---- 3. TimePoint 算术操作 ----
    // 设计意图：add/since 仅涉及 Rep 整数运算，无系统调用
    {
        auto tp = time::nowSystem();
        BENCH_N("TimePoint add<MSEC>(1000)", {
            tp.add<time::MSEC>(1000);
        });
        BENCH_N("TimePoint since<MSEC>(tp)", {
            volatile auto diff = tp.since<time::MSEC>(tp);
            (void)diff;
        });
    }
    BENCH_SEP();

    // ---- 4. TimePoint 转 DateTime ----
    // 设计意图：localTime/utcTime 调用系统 time_t→tm 转换。
    // localTime 含 TZ 计算，utcTime 用 gmtime_r 更轻量。
    {
        auto tp = time::nowSystem();
        BENCH_N("TimePoint localTime() → DateTime", {
            volatile auto dt = tp.localTime();
            (void)dt;
        });
        BENCH_N("TimePoint utcTime() → DateTime", {
            volatile auto dt = tp.utcTime();
            (void)dt;
        });
    }
    BENCH_SEP();

    // ---- 5. fromTimeValue / toTimeValue ----
    // 设计意图：纯赋值/读取操作，编译器可能完全优化
    {
        time::TimePoint tp;
        BENCH_N_CONSTEXPR("TimePoint fromTimeValue<MSEC>(12345)", {
            tp.fromTimeValue<time::MSEC>(12345);
        });
        BENCH_N("TimePoint toTimeValue<MSEC>()", {
            volatile auto v = tp.toTimeValue<time::MSEC>();
            (void)v;
        });
    }
    BENCH_SEP();

    // ================================================================
    // 计时工具精度分析
    // ================================================================

    // ---- 6. 两次 steady_clock::now() 开销（计时工具的核心） ----
    // 设计意图：所有计时工具的本质都是两个 clock 相减，这个操作本身的精度下限
    {
        auto t1 = std::chrono::steady_clock::now();
        BENCH_N("两次 steady_clock::now() 相减", {
            auto a = std::chrono::steady_clock::now();
            auto b = std::chrono::steady_clock::now();
            volatile auto diff = b - a;
            (void)diff;
        });
        (void)t1;
    }
    BENCH_SEP();

    // ---- 7. TimeCounterScope 禁用日志时的开销 ----
    // 设计意图：关闭全局计时计数器后，TimeCounterScope 的析构不输出日志，
    // 只做 deinit() 中的判断 + now() 计算。这是"纯计时"开销。
    {
        // 先关闭日志输出
        time::enableGlobalTimeCounter(false);
        BENCH_N("TimeCounterScope (禁用日志, 纯计时开销)", {
            time::TimeCounterScope<time::MSEC> guard("test");
            volatile int sink = 0;
            (void)sink;
        });

        // 恢复
        time::enableGlobalTimeCounter(true);
    }
    BENCH_SEP();

    // ---- 8. TimeCounterScope 启用日志时的完整开销 ----
    // 设计意图：包含 MEIDO_INFO 日志路径（format+时间戳+原子+回调），完整开销。
    // 用无操作回调抑制输出（printf 写 stdout，cout.rdbuf 重定向拦不住它）。
    {
        log::resetLogger();
        log::setupSyncLogger(log::Level::INFO, [](log::Message) {});
        BENCH_N("TimeCounterScope (启用日志, 完整开销)", {
            time::TimeCounterScope<time::MSEC> guard("test");
            volatile int sink = 0;
            (void)sink;
        });
        log::resetLogger();
    }
    BENCH_SEP();

    // ---- 9. MeanTimeCounter markScope 开销 ----
    // 设计意图：MeanTimeCounter 每次 markScope 返回 ScopeGuard，
    // 内部通过 markStart/markEnd 更新统计窗口
    {
        time::MeanTimeCounter mtc(1000, false);    // 禁用，不统计
        BENCH_N("MeanTimeCounter markScope (禁用, 空操作)", {
            auto guard = mtc.markScope("test");
            volatile int sink = 0;
            (void)sink;
        });

        time::MeanTimeCounter mtc2(1000000, true);    // 启用，大窗口避免滚动
        BENCH_N("MeanTimeCounter markScope (启用, 全路径)", {
            auto guard = mtc2.markScope("test");
            volatile int sink = 0;
            (void)sink;
        });
    }
    BENCH_SEP();

    // ---- 6. sleep 短时间 ----
    {
        BENCH_N("usleep(1000) 1ms 精度 (N=100)", {
            time::usleep(1000);
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidotimebench

#endif // BENCH_TIME_HPP

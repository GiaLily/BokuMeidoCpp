/*  time 模块单元测试
    分类：时间点与时钟 | 计时与守卫  */
#pragma once
#ifndef TIME_TEST_HPP_BOKUMEIDOCPP
#define TIME_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/time.hpp"

using namespace meido;
namespace _meidotimecheck
{

inline void MeanTimeCounterTest()
{
    time::MeanTimeCounter time_counter{10, true};
    {
        // Guard 2 先完成10次，Guard 1 再完成10次
        for (int i = 0; i < 10; i++)
        {
            auto guard = time_counter.markScope("Guard 2");
            time::msleep(60);
        }
        for (int i = 0; i < 10; i++)
        {
            auto guard = time_counter.markScope("Guard 1");
            time::msleep(60);
        }

        // 窗口已达到，get 返回有效值
        auto cost_g2 = time_counter.getMeanTimeCost<time::MSEC>("Guard 2", true);
        auto cost_g1 = time_counter.getMeanTimeCost<time::MSEC>("Guard 1", true);
        MEIDO_ASSERT(cost_g2 >= 50);
        MEIDO_ASSERT(cost_g1 >= 50);

        // 对齐输出，展示 vs 期望的 60ms
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Author check! Guard 2 mean cost time: Expected| ~60ms in 10 counts");
            _MEIDO_INFO_RAW(buf);
            snprintf(buf, sizeof(buf), "              Guard 2 mean cost time:   Actual| %lldms in 10 counts", (long long)cost_g2);
            _MEIDO_INFO_RAW(buf);
        }
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Author check! Guard 1 mean cost time: Expected| ~60ms in 10 counts");
            _MEIDO_INFO_RAW(buf);
            snprintf(buf, sizeof(buf), "              Guard 1 mean cost time:   Actual| %lldms in 10 counts", (long long)cost_g1);
            _MEIDO_INFO_RAW(buf);
        }

        // window_reached_only=false 时仍可获取
        MEIDO_ASSERT(time_counter.getMeanTimeCost<time::MSEC>("Guard 2", false) >= 50);
        MEIDO_ASSERT(time_counter.getMeanTimeCost<time::MSEC>("Guard 1", false) >= 50);

        // 再次 window_reached_only=true 应返回 -1
        MEIDO_ASSERT(time_counter.getMeanTimeCost<time::MSEC>("Guard 2", true) == -1);
        MEIDO_ASSERT(time_counter.getMeanTimeCost<time::MSEC>("Guard 1", true) == -1);
    }
}

inline void nowSystemTest()
{
    auto start_t = time::nowSystem();
    time::msleep(60);
    auto end_t = time::nowSystem();
    auto elapsed = end_t.since<time::MSEC>(start_t);
    MEIDO_ASSERT(elapsed >= 50);

    char buf[256];
    snprintf(buf, sizeof(buf), "Author check! nowSystem sleep 60ms: Expected| >= 50ms");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              nowSystem sleep 60ms:   Actual| %lldms", (long long)elapsed);
    _MEIDO_INFO_RAW(buf);
}

inline void nowSteadyTest()
{
    auto tp1 = time::nowSteady();
    time::msleep(10);
    auto tp2 = time::nowSteady();
    MEIDO_ASSERT(tp2.since<time::NSEC>(tp1) > 0);
}

inline void minTimeScopeTest()
{
    auto start_t = time::nowSystem();
    {
        time::MinTimeScope<time::USEC> guard(60 * 1000, 1);
        guard.reset(60 * 1000, 1);
    }
    auto elapsed = time::nowSystem().since<time::MSEC>(start_t);
    // 两个60ms段总计，系统调度可能稍长
    MEIDO_ASSERT(elapsed >= 110);

    char buf[256];
    snprintf(buf, sizeof(buf), "Author check! MinTimeScope 60ms+60ms: Expected| >= 110ms");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              MinTimeScope 60ms+60ms:   Actual| %lldms", (long long)elapsed);
    _MEIDO_INFO_RAW(buf);
}

inline void timePointTest()
{
    // fromTimeValue + toTimeValue 往返
    time::TimePoint tp;
    tp.fromTimeValue<time::MSEC>(12345);
    MEIDO_ASSERT(tp.toTimeValue<time::MSEC>() == 12345);
    MEIDO_ASSERT(tp.toTimeValue<time::SEC>() == 12);    // 12345ms = 12s

    // add 加法
    tp.add<time::MSEC>(1000);
    MEIDO_ASSERT(tp.toTimeValue<time::MSEC>() == 13345);

    // add 减法
    tp.add<time::MSEC>(-500);
    MEIDO_ASSERT(tp.toTimeValue<time::MSEC>() == 12845);

    // 超大值截断
    time::TimePoint tp_max;
    tp_max.fromTimeValue<time::NSEC>(std::chrono::nanoseconds::max().count() / 2);
    tp_max.add<time::NSEC>(std::chrono::nanoseconds::max().count() / 2 + 1);
    MEIDO_ASSERT(tp_max.toTimeValue<time::NSEC>() == std::chrono::nanoseconds::max().count());

    // 超小值截断
    time::TimePoint tp_min;
    tp_min.fromTimeValue<time::NSEC>(std::chrono::nanoseconds::min().count() / 2);
    printf("Author check! Expected| [WARN] Addition underflow detected (min value truncation test)\n");
    tp_min.add<time::NSEC>(std::chrono::nanoseconds::min().count() / 2 - 1);
    MEIDO_ASSERT(tp_min.toTimeValue<time::NSEC>() == std::chrono::nanoseconds::min().count());

    // DateTime 边界值输出（仅供人工验证）
    MEIDO_INFO_RAW("Author check! DateTime boundary output:");
    auto now_tp = time::nowSystem();
    MEIDO_INFO_RAW("  Local time (now) = {}", now_tp.localTime());
    MEIDO_INFO_RAW("  UTC time   (now) = {}", now_tp.utcTime());

    printf("Author check! Expected| [WARN] Param time_value too small, set to min value (boundary test)\n");
    time::DateTime min_utc = time::TimePoint().fromTimeValue<time::USEC>(std::chrono::nanoseconds::min().count()).utcTime();
    printf("Author check! Expected| [WARN] Param time_value too large, set to max value (boundary test)\n");
    time::DateTime max_utc = time::TimePoint().fromTimeValue<time::USEC>(std::chrono::nanoseconds::max().count()).utcTime();
    MEIDO_INFO_RAW("  Min UTC (timestamp min) = {}", min_utc);
    MEIDO_INFO_RAW("  Max UTC (timestamp max) = {}", max_utc);
    MEIDO_INFO_RAW("  Invalid (default constructed) = {}", time::DateTime());

    // DateTime operator<< 单次输出回归测试（防止双写回归：write 后再次 sputn）
    {
        std::ostringstream oss;
        oss << time::DateTime();
        MEIDO_ASSERT(oss.str() == "0000-00-00 00:00:00 Invalid");
    }

    // since 基本测试
    time::TimePoint tp_a, tp_b;
    tp_a.fromTimeValue<time::MSEC>(1000);
    tp_b.fromTimeValue<time::MSEC>(1050);
    MEIDO_ASSERT(tp_b.since<time::MSEC>(tp_a) == 50);
    MEIDO_ASSERT(tp_a.since<time::MSEC>(tp_b) == -50);    // 负值
}

inline void timeCounterScopeTest()
{
    // 基本构造→析构，展示预期 vs 实际
    {
        auto tp = time::nowSystem();
        {
            printf("Author check! Expected| TimeCounterScope: time counter guard cost time ~30ms (basic scope test)\n");
            time::TimeCounterScope<time::MSEC> guard("time counter guard");
            time::msleep(30);
        }
        auto elapsed = time::nowSystem().since<time::MSEC>(tp);
        MEIDO_ASSERT(elapsed >= 25);

        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! TimeCounterScope 30ms: Expected| ~30ms");
        _MEIDO_INFO_RAW(buf);
        snprintf(buf, sizeof(buf), "              TimeCounterScope 30ms:   Actual| %lldms", (long long)elapsed);
        _MEIDO_INFO_RAW(buf);
    }
    // reset 重新计时
    {
        auto tp = time::nowSystem();
        {
            printf("Author check! Expected| TimeCounterScope: time counter guard reset cost time ~30ms (reset re-timing test)\n");
            time::TimeCounterScope<time::MSEC> guard("time counter guard reset");
            time::msleep(30);
            printf("Author check! Expected| TimeCounterScope: time counter guard reset 2 cost time ~30ms (reset re-timing test)\n");
            guard.reset("time counter guard reset 2");
            time::msleep(30);
        }
        auto elapsed = time::nowSystem().since<time::MSEC>(tp);
        MEIDO_ASSERT(elapsed >= 55);

        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! TimeCounterScope reset 30msx2: Expected| ~60ms");
        _MEIDO_INFO_RAW(buf);
        snprintf(buf, sizeof(buf), "              TimeCounterScope reset 30msx2:   Actual| %lldms", (long long)elapsed);
        _MEIDO_INFO_RAW(buf);
    }
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check time start--------------------");

    // ---- 1. 时间点与时钟 ----
    MeanTimeCounterTest();
    nowSystemTest();
    nowSteadyTest();

    // ---- 2. 计时与守卫 ----
    minTimeScopeTest();
    timePointTest();
    timeCounterScopeTest();

    _MEIDO_INFO_RAW("---------------------check time end---------------------\n\n");
}

}    // namespace _meidotimecheck

#endif    // !TIME_TEST_HPP_BOKUMEIDOCPP

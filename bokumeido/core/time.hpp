/*  bokumeidocpp库的便利时间相关工具  */
#pragma once
#ifndef TIME_HPP_BOKUMEIDOCPP
#define TIME_HPP_BOKUMEIDOCPP

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "base.hpp"
#include "log.hpp"
#include "str.hpp"

namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

// 基于<chrono>库的简易计时函数封装
namespace time
{
    // 用于表示时间的存储数值类型，通常是long long
    using Rep = std::chrono::nanoseconds::rep;
    static_assert(std::is_signed<Rep>::value, "Rep type must be signed type!");

    // using TimeValue
    // 时间单位
    enum Unit : Rep
    {
        NSEC = 1,
        USEC = 1000,
        MSEC = 1000 * 1000,
        SEC = 1000 * 1000 * 1000
    };

    // 日期时间信息，包含常用的日期时间字段和自纪元以来的时间戳成员
    struct DateTime
    {
        int year = 0;      // 1900-现今
        int month = 0;     // 1-12
        int mday = 0;      // 1-31
        int hour = 0;      // 0-23
        int minute = 0;    // 0-59
        int second = 0;    // 0-60

        int isdst = 0;         // 1代表夏令时，0代表非夏令时，其他代表未知
        bool isutc = false;    // UTC时间or当地时间
        bool valid = false;    // 时间数据是否有效，超出C标准库tm能表示的范围时无效
    };

    /*  为DateTime对象添加对 operator<< 的支持
        - 按照"2025-01-07 17:18:35 UTC"的格式，UTC根据实际可能为DST、Local、Invalid等  */
    std::ostream& operator<<(std::ostream& os_obj, const DateTime& date_time);


    // 时间点类，合法范围是以纳秒为单位时的Rep类型最小值到最大值，超出范围时会被截断到最小或最大值
    class TimePoint final
    {
    public:
        // 创建一个0时刻的时间点
        TimePoint() = default;

        // 通过指定时间值和单位创建时间点
        template <Unit unit>
        TimePoint& fromTimeValue(Rep time_value);

        template <Unit unit>
        Rep toTimeValue() const;

        /*  将本时间点转换为日期时间
            - 因为chrono和time_t的有效时间范围并不保证一致，因此转换可能失败，通过DateTime的valid成员判断转换是否成功
            - DateTime的有效性仅代表是否在time_t有效时间范围内转换，不会区分时间点的值来源是否为系统时间  */
        DateTime localTime() const;
        DateTime utcTime() const;

        // 根据unit计算从tp到本时间点之间的时长，溢出时会被截断到最小或最大值
        template <Unit unit>
        Rep since(const TimePoint& tp) const;

        // 在当前时间值上叠加时间段，正负均可，但结果超出合法范围时会被截断到最小或最大值
        template <Unit unit>
        TimePoint& add(Rep duration);

    private:
        DateTime tmToDateTime(const tm& timeinfo, bool is_utc) const;

        Rep timeval_ = 0;    // 以ns为单位的时间值
    };

    // 获得当前系统时间点
    TimePoint nowSystem();

    // 获取当前稳态时间点
    TimePoint nowSteady();

    // 线程休眠(秒)
    void sleep(Rep t);

    // 线程休眠(毫秒)
    void msleep(Rep t);

    // 线程休眠(微秒)
    void usleep(Rep t);

    // 线程休眠(纳秒)
    void nsleep(Rep t);

    // 只有开启时TimeCounter系列类的统计功能才生效，作用范围为当前二进制模块
    void enableGlobalTimeCounter(bool enabled);

    // 用于统计各个代码段的在一定循环次数的平均消耗时间，非线程安全
    class MeanTimeCounter final
    {
    private:
        class MarkEnd;

    public:
        MeanTimeCounter();
        /*  构造MeanTimeCounter类
            @param count_window: 统计窗口，小于1的值会被置为1
            @param enabled: 是否开启计时功能
            @param unit: 计时单位  */
        MeanTimeCounter(Rep count_window, bool enabled = true);

        /*  本轮统计开始，应在目标统计代码段前调用，与段后markEnd成对出现
            @param codeblock_tag: 要统计的代码段的tag   */
        void markStart(const std::string& codeblock_tag);

        /*  本轮统计结束，应在目标统计代码段后调用，与段前markStart成对出现
            @param codeblock_tag: 要统计的代码段的tag   */
        void markEnd(const std::string& codeblock_tag);

        /*  安全记录一段代码的耗时，调用时记录开始时间，ScopeGuard对象析构时记录结束时间
            - 用例：auto guard = time_counter.markScope("codeblock_tag")
            @param codeblock_tag: 要统计的代码段的tag
            @return 一个ScopeGuard对象，只能用auto推导  */
        base::ScopeGuard<MarkEnd> markScope(const std::string& codeblock_tag);

        /*  获取最近一次统计窗口的平均时长
            @param codeblock_tag: 要统计的代码段的tag
            @param window_reached_only: 为true时仅在统计窗口达到时返回有效值，未达到时返回负数
            @return 平均消耗时间，单位由模板参数unit指定，负数代表未获取到有效值(首次统计窗口还未达到、codeblock_tag不存在等)  */
        template <Unit unit>
        Rep getMeanTimeCost(const std::string& codeblock_tag, bool window_reached_only = false);

        /*  在每个被统计的代码段达到目标统计次数后，打印其平均消耗时间并重新开始统计此段代码  */
        template <Unit unit>
        void logOnWindowReached();

        // 支持移动禁止拷贝
        MeanTimeCounter(MeanTimeCounter&& rvalue) = default;
        MeanTimeCounter& operator=(MeanTimeCounter&& rvalue) = default;

    private:
        class SingleCounter;

        Rep count_window_ = 1;
        bool self_enabled_ = true;
        bool final_enabled_ = true;

        // 有依赖关系
        std::vector<std::string> tags_;    // 用于存储插入顺序，
        std::unique_ptr<std::map<std::string, SingleCounter>> counter_map_;
    };

    /*  统计并打印该对象从获取资源到释放资源之间的时间消耗
        - 用例：TimeCounterScope<time::MSEC> guard("tag");    //获取资源
        - 用例：guard.reset("tag");    //释放旧资源，获取新资源  */
    template <Unit unit>
    class TimeCounterScope final
    {
    public:
        // 构造一个空的对象，无资源
        TimeCounterScope() = default;

        /*  构造对象，获取资源，统计从当前到资源释放之间的时间消耗
            @param codeblock_tag: 要计时的代码块标识符  */
        TimeCounterScope(std::string codeblock_tag);

        // 提前释放资源并将对象置空
        void reset();
        // 释放旧资源，获取新资源
        void reset(std::string codeblock_tag);

        // 支持移动构造，禁止赋值和拷贝构造
        TimeCounterScope(TimeCounterScope<unit>&& obj) noexcept;
        TimeCounterScope<unit>& operator=(const TimeCounterScope<unit>& obj) = delete;
        // 释放资源
        ~TimeCounterScope();

    private:
        void init(std::string& codeblock_tag);
        void deinit();

        bool valid_ = false;
        std::chrono::steady_clock::time_point start_t_{};
        std::string codeblock_tag_;
    };

    /*  设置从获取资源到释放资源的代码段的最短时间
        - 获取资源的时间+duration的合法范围是以纳秒为单位时Rep的最小值和最大值之间，如果超出合法范围会被截断到最小或最大值
        - 用例1：MinTimeScope<time::MSEC> guard(50);    //获取资源，保证从当前到释放资源之间的代码段时间不低于50毫秒
        - 用例2：guard.reset(50);    //释放旧资源，获取新资源  */
    template <Unit unit>
    class MinTimeScope final
    {
    public:
        // 构造空对象，无资源
        MinTimeScope() = default;

        /*  构造对象，获取资源，确保从当前到资源释放之间的代码段时间不低于duration
            @param duration: 目标时长
            @param sleep_interval: 内部循环sleep的间隔，以unit为单位，负数时固定1ms，大于duration时置为duration
            @param quit_pred: 谓词回调，返回true时提前释放资源  */
        MinTimeScope(Rep duration, Rep sleep_interval = -1, std::function<bool()> quit_pred = {});

        // 提前释放资源并将对象置空，释放资源时会等待剩余时间
        void reset();
        // 释放旧资源，获取新资源，释放资源时会等待剩余时间
        void reset(Rep duration, Rep sleep_interval = -1, std::function<bool()> quit_pred = {});

        // 支持移动构造，禁止拷贝构造，禁止赋值
        MinTimeScope(MinTimeScope<unit>&& obj) noexcept;
        MinTimeScope<unit>& operator=(const MinTimeScope<unit>& obj) = delete;
        // 释放资源
        ~MinTimeScope();

    private:
        void init(Rep duration, Rep sleep_interval = -1, std::function<bool()> quit_pred = {});
        void deinit();

        bool valid_ = false;
        Rep sleep_interval_ = time::MSEC;
        std::chrono::steady_clock::time_point target_tp_{};
        std::function<bool()> quit_pred_;
    };

}    // namespace time

/*--------------------------------------------内部实现--------------------------------------------*/

namespace _priv
{
    using Rep = time::Rep;

    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    inline T safeAdd(T a, T b, const char* func_name)
    {
        if (b > 0 && a > std::numeric_limits<T>::max() - b)
        {
            MEIDO_WARN("Addition overflow detected in {}! Returned max value", func_name);
            return std::numeric_limits<T>::max();    // 正溢出
        }

        if (b < 0 && a < std::numeric_limits<T>::min() - b)
        {
            MEIDO_WARN("Addition underflow detected in {}! Returned min value", func_name);
            return std::numeric_limits<T>::min();    // 负溢出
        }
        return a + b;    // 不会溢出
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline T safeAdd(T a, T b, const char* func_name)
    {
        return a + b;
    }

    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    inline T safeSub(T a, T b, const char* func_name)
    {
        if (b > 0 && a < std::numeric_limits<T>::min() + b)
        {
            MEIDO_WARN("Subtraction underflow detected in {}! Returned min value", func_name);
            return std::numeric_limits<T>::min();    // 负溢出
        }
        if (b < 0 && a > std::numeric_limits<T>::max() + b)
        {
            MEIDO_WARN("Subtraction overflow detected in {}! Returned max value", func_name);
            return std::numeric_limits<T>::max();    // 正溢出
        }
        return a - b;    // 不会溢出
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline T safeSub(T a, T b, const char* func_name)
    {
        return a - b;    // 不会溢出
    }

    // 检查chrono::system_clock::nanoseconds / time_t的倍率关系，如果返回值小于1则代表异常
    inline Rep checkAndCountRatioOfTimeT()
    {
        if (sizeof(Rep) < sizeof(int64_t))
        {
            MEIDO_WARN("Rep type too small to calculate ratio with time_t");
            return 0;    // 类型过小不支持
        }
        if (!std::is_integral<Rep>::value || !std::is_integral<time_t>::value)
        {
            MEIDO_WARN("Rep or time_t type is not integral type, cannot calculate ratio with time_t");
            return 0;    // 非整型不支持
        }
        // 注：不能直接用 nanoseconds(0) 做基准——MSVC 的 system_clock 可能使用
        // FILETIME 纪元（1601-01-01），to_time_t() 对 1970 年之前的时间点返回 -1。
        // 用 now() 做基准确保 to_time_t() 总是有效的。
        auto now_ns = std::chrono::system_clock::now();

        auto ns0 = now_ns + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(0));
        auto ns1 = now_ns + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(1000 * 1000 * 1000));
        auto time_t_val0 = std::chrono::system_clock::to_time_t(ns0);
        auto time_t_val1 = std::chrono::system_clock::to_time_t(ns1);
        if (time_t_val1 <= time_t_val0)
        {
            MEIDO_ERROR("Unexpected time_t difference direction, cannot calculate ratio");
            return 0;
        }
        Rep ratio_a = static_cast<Rep>(1000 * 1000 * 1000 / (time_t_val1 - time_t_val0));

        ns0 = now_ns + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(10 * 1000 * 1000 * 1000LL));
        ns1 = now_ns + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(20 * 1000 * 1000 * 1000LL));
        time_t_val0 = std::chrono::system_clock::to_time_t(ns0);
        time_t_val1 = std::chrono::system_clock::to_time_t(ns1);
        if (time_t_val1 <= time_t_val0)
        {
            MEIDO_ERROR("Unexpected time_t difference direction, cannot calculate ratio");
            return 0;
        }
        Rep ratio_b = static_cast<Rep>(10 * 1000 * 1000 * 1000LL / (time_t_val1 - time_t_val0));

        if (ratio_a != ratio_b || ratio_a < 1)
        {
            MEIDO_WARN("Detected unusual ratio between std::chrono::system_clock::time_point and time_t, time_t may have low resolution");
            return 0;
        }

        return ratio_b;
    }
}    // namespace _priv

namespace time
{

    inline std::ostream& operator<<(std::ostream& os_obj, const DateTime& date_time)
    {
        char buf[32];
        size_t pos = 0;

        _priv::write4d(buf + pos, date_time.year);
        pos += 4;
        buf[pos++] = '-';
        _priv::write2d(buf + pos, date_time.month);
        pos += 2;
        buf[pos++] = '-';
        _priv::write2d(buf + pos, date_time.mday);
        pos += 2;
        buf[pos++] = ' ';
        _priv::write2d(buf + pos, date_time.hour);
        pos += 2;
        buf[pos++] = ':';
        _priv::write2d(buf + pos, date_time.minute);
        pos += 2;
        buf[pos++] = ':';
        _priv::write2d(buf + pos, date_time.second);
        pos += 2;
        buf[pos++] = ' ';

        const char* suffix = nullptr;
        size_t suffix_len = 0;
        if (!date_time.valid)
        {
            suffix = "Invalid";
            suffix_len = 7;
        }
        else if (date_time.isutc)
        {
            suffix = "UTC";
            suffix_len = 3;
        }
        else if (date_time.isdst)
        {
            suffix = "DST";
            suffix_len = 3;
        }
        else
        {
            suffix = "Local";
            suffix_len = 5;
        }
        memcpy(buf + pos, suffix, suffix_len);
        pos += suffix_len;

        os_obj.rdbuf()->sputn(buf, static_cast<std::streamsize>(pos));
        return os_obj;
    }

    inline DateTime TimePoint::localTime() const
    {
        static Rep time_t_ratio = _priv::checkAndCountRatioOfTimeT();
        if (time_t_ratio < 1)
        {
            MEIDO_WARN("Cannot convert TimePoint to local DateTime due to invalid time_t ratio");
            return DateTime();
        }
        if (timeval_ / time_t_ratio >= std::numeric_limits<time_t>::max() || timeval_ / time_t_ratio <= std::numeric_limits<time_t>::min())
        {
            MEIDO_WARN("TimePoint value out of time_t range, cannot convert to local DateTime");
            return DateTime();
        }

        std::chrono::system_clock::time_point tp(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(timeval_)));
        time_t sys_time_t = std::chrono::system_clock::to_time_t(tp);
        tm buf;
#if defined(_MEIDO_WIN32)
        if (::localtime_s(&buf, &sys_time_t) != 0)
#else
        if (::localtime_r(&sys_time_t, &buf) == nullptr)
#endif
        {
            MEIDO_WARN("Got Invalid DateTime!\n");
            return DateTime();
        }
        return tmToDateTime(buf, false);
    }

    inline DateTime TimePoint::utcTime() const
    {
        static Rep time_t_ratio = _priv::checkAndCountRatioOfTimeT();
        if (time_t_ratio < 1)
        {
            MEIDO_WARN("Cannot convert TimePoint to local DateTime due to invalid time_t ratio");
            return DateTime();
        }
        if (timeval_ / time_t_ratio >= std::numeric_limits<time_t>::max() || timeval_ / time_t_ratio <= std::numeric_limits<time_t>::min())
        {
            MEIDO_WARN("TimePoint value out of time_t range, cannot convert to local DateTime");
            return DateTime();
        }

        std::chrono::system_clock::time_point tp(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(timeval_)));
        time_t sys_time_t = std::chrono::system_clock::to_time_t(tp);
        tm buf;
#if defined(_MEIDO_WIN32)
        if (::gmtime_s(&buf, &sys_time_t) != 0)
#else
        if (::gmtime_r(&sys_time_t, &buf) == nullptr)
#endif
        {
            MEIDO_WARN("Got Invalid DateTime");
            return DateTime();
        }
        return tmToDateTime(buf, true);
    }

    template <Unit unit>
    TimePoint& TimePoint::fromTimeValue(Rep time_value)
    {
        if (time_value < std::chrono::nanoseconds::min().count() / unit)
        {
            timeval_ = std::chrono::nanoseconds::min().count();
            MEIDO_WARN("Param time_value too small, set to min value");
        }
        else if (time_value > std::chrono::nanoseconds::max().count() / unit)
        {
            timeval_ = std::chrono::nanoseconds::max().count();
            MEIDO_WARN("Param time_value too large, set to max value");
        }
        else
            timeval_ = time_value * unit;
        return *this;
    }

    template <Unit unit>
    inline Rep TimePoint::toTimeValue() const
    {
        return timeval_ / unit;
    }

    template <Unit unit>
    inline Rep TimePoint::since(const TimePoint& tp) const
    {
        Rep duration = _priv::safeSub(timeval_, tp.timeval_, MEIDO_FUNCNAME);
        return duration / unit;
    }

    template <Unit unit>
    TimePoint& TimePoint::add(Rep duration)
    {
        if (duration < std::chrono::nanoseconds::min().count() / unit)
        {
            duration = std::chrono::nanoseconds::min().count();
            MEIDO_WARN("Param duration too small, set to min supported value");
        }
        else if (duration > std::chrono::nanoseconds::max().count() / unit)
        {
            duration = std::chrono::nanoseconds::max().count();
            MEIDO_WARN("Param duration too large, set to max supported value");
        }
        else
            duration = duration * unit;
        timeval_ = _priv::safeAdd(timeval_, duration, MEIDO_FUNCNAME);
        return *this;
    }

    inline DateTime TimePoint::tmToDateTime(const tm& timeinfo, bool is_utc) const
    {
        DateTime date_time;
        date_time.year = timeinfo.tm_year + 1900;
        date_time.month = timeinfo.tm_mon + 1;
        date_time.mday = timeinfo.tm_mday;
        date_time.hour = timeinfo.tm_hour;
        date_time.minute = timeinfo.tm_min;
        date_time.second = timeinfo.tm_sec;
        date_time.isutc = is_utc;
        if (is_utc)
            date_time.isdst = 0;
        else
            date_time.isdst = timeinfo.tm_isdst;
        date_time.valid = true;
        return date_time;
    }

    inline TimePoint nowSystem()
    {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        TimePoint systime;
        systime.fromTimeValue<time::NSEC>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
        return systime;
    }

    inline TimePoint nowSteady()
    {
        auto duration = std::chrono::steady_clock::now().time_since_epoch();
        TimePoint steadytime;
        steadytime.fromTimeValue<time::NSEC>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
        return steadytime;
    }

    // 线程休眠(秒)
    inline void sleep(Rep t)
    {
        if (t > 0)
            std::this_thread::sleep_for(std::chrono::seconds(t));
    }

    // 线程休眠(毫秒)
    inline void msleep(Rep t)
    {
        if (t > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    // 线程休眠(微秒)
    inline void usleep(Rep t)
    {
        if (t > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    // 线程休眠(纳秒)
    inline void nsleep(Rep t)
    {
        if (t > 0)
            std::this_thread::sleep_for(std::chrono::nanoseconds(t));
    }

    inline std::atomic<bool>& _getGlobalTimeCounterEnabled()
    {
        static std::atomic<bool> g_timecounter_on(true);
        return g_timecounter_on;
    }

    inline void enableGlobalTimeCounter(bool enabled)
    {
        time::_getGlobalTimeCounterEnabled().store(enabled, std::memory_order_relaxed);
    }

    class MeanTimeCounter::SingleCounter final
    {
    public:
        SingleCounter() = default;

        explicit SingleCounter(Rep count_window)
        {
            count_window_ = count_window;
        }

        void markStart()
        {
            start_t_ = std::chrono::steady_clock::now();
            markstart_counts_ += 1;
        }

        void markEnd(const std::string& codeblock_tag)
        {
            time_cost_ += (std::chrono::steady_clock::now() - start_t_);
            markend_counts_ += 1;
            if (markend_counts_ != markstart_counts_)
            {
                MEIDO_WARN("Current counts of {}: markStart={}, markEnd={}. Please check your code", codeblock_tag, markstart_counts_, markend_counts_);
                return;
            }
            if (markend_counts_ >= count_window_)
            {
                mean_time_cost_ = std::chrono::duration_cast<std::chrono::nanoseconds>(time_cost_).count() / markend_counts_;
                this->restart();
                can_get_ = true;
                can_log_ = true;
            }
        }

        template <Unit unit>
        Rep getMeanTimeCost(bool window_reached_only)
        {
            if (mean_time_cost_ < 0)
                return -1;
            if (window_reached_only && !can_get_)
                return -1;
            can_get_ = false;
            return mean_time_cost_ / unit;
        }

        template <Unit unit>
        void logOnWindowReached(const std::string& codeblock_tag)
        {
            if (!can_log_)
                return;
            auto mean_time_cost = mean_time_cost_ / unit;
            const char* unit_str = "unknown unit";
            switch (unit)
            {
            case Unit::NSEC:
                unit_str = "ns";
                break;
            case Unit::USEC:
                unit_str = "us";
                break;
            case Unit::MSEC:
                unit_str = "ms";
                break;
            case Unit::SEC:
                unit_str = "s";
                break;
            }
            MEIDO_INFO_RAW("{} mean cost time {} {} in {} counts", codeblock_tag, mean_time_cost, unit_str, count_window_);
            can_log_ = false;
        }

        SingleCounter(SingleCounter&&) noexcept = default;
        SingleCounter& operator=(SingleCounter&&) noexcept = default;

    private:
        void restart()
        {
            markstart_counts_ = 0;
            markend_counts_ = 0;
            time_cost_ = std::chrono::nanoseconds(0);
        }

        Rep markstart_counts_ = 0;
        Rep markend_counts_ = 0;
        std::chrono::nanoseconds time_cost_{0};
        std::chrono::steady_clock::time_point start_t_{};
        Rep mean_time_cost_ = -1;    // 纳秒
        std::string log_fmt_;

        Rep count_window_ = 1;
        bool can_get_ = false;
        bool can_log_ = false;
    };

    class MeanTimeCounter::MarkEnd
    {
    public:
        void operator()()
        {
            if (single_counter_)
                single_counter_->markEnd(*tag_);
        }

        MarkEnd(MarkEnd&& tmp) noexcept
        {
            tag_ = tmp.tag_;
            single_counter_ = tmp.single_counter_;
        }
        MarkEnd& operator=(const MarkEnd&) = delete;

    private:
        MarkEnd(const std::string* tag, SingleCounter* single_counter) :
            tag_(tag),
            single_counter_(single_counter)
        {}

        const std::string* tag_ = nullptr;
        SingleCounter* single_counter_ = nullptr;
        friend MeanTimeCounter;
    };

    inline MeanTimeCounter::MeanTimeCounter()
    {
        count_window_ = 1;
        self_enabled_ = true;
        final_enabled_ = time::_getGlobalTimeCounterEnabled().load(std::memory_order_acquire);
        counter_map_.reset(new std::map<std::string, SingleCounter>);
    }

    inline MeanTimeCounter::MeanTimeCounter(Rep count_window, bool enabled)
    {
        if (count_window < 1)
        {
            MEIDO_WARN("Param count_window {} invalid, set to 1", count_window);
            count_window = 1;
        }
        count_window_ = count_window;
        self_enabled_ = enabled;
        final_enabled_ = time::_getGlobalTimeCounterEnabled().load(std::memory_order_acquire) && enabled;
        counter_map_.reset(new std::map<std::string, SingleCounter>);
    }

    inline void MeanTimeCounter::markStart(const std::string& codeblock_tag)
    {
        if (!final_enabled_)
            return;
        auto it = counter_map_->find(codeblock_tag);
        if (counter_map_->end() == it)
        {
            tags_.emplace_back(codeblock_tag);
            it = counter_map_->emplace(codeblock_tag, SingleCounter(count_window_)).first;
        }
        it->second.markStart();
    }

    inline void MeanTimeCounter::markEnd(const std::string& codeblock_tag)
    {
        if (!final_enabled_)
            return;
        auto it = counter_map_->find(codeblock_tag);
        if (counter_map_->end() == it)
        {
            MEIDO_WARN("No matched markStart({}) for markEnd({})", codeblock_tag, codeblock_tag);
            return;
        }
        it->second.markEnd(codeblock_tag);
    }

    inline base::ScopeGuard<MeanTimeCounter::MarkEnd> MeanTimeCounter::markScope(const std::string& codeblock_tag)
    {
        if (!final_enabled_)
            return base::makeScopeGuard(MarkEnd(nullptr, nullptr));

        auto it = counter_map_->find(codeblock_tag);
        if (counter_map_->end() == it)
        {
            tags_.emplace_back(codeblock_tag);
            it = counter_map_->emplace(codeblock_tag, SingleCounter(count_window_)).first;
        }
        it->second.markStart();
        return base::makeScopeGuard(MarkEnd(&it->first, &it->second));
    }

    template <Unit unit>
    inline Rep MeanTimeCounter::getMeanTimeCost(const std::string& codeblock_tag, bool window_reached_only)
    {
        final_enabled_ = time::_getGlobalTimeCounterEnabled().load(std::memory_order_relaxed) && self_enabled_;
        if (!final_enabled_)
            return -1;
        auto it = counter_map_->find(codeblock_tag);
        if (counter_map_->end() == it)
        {
            MEIDO_WARN("No such codeblock_tag:{} found in MeanTimeCounter", codeblock_tag);
            return -1;
        }
        return it->second.getMeanTimeCost<unit>(window_reached_only);
    }

    template <Unit unit>
    inline void MeanTimeCounter::logOnWindowReached()
    {
        final_enabled_ = time::_getGlobalTimeCounterEnabled().load(std::memory_order_relaxed) && self_enabled_;
        if (!final_enabled_)
            return;
        for (const std::string& tag : tags_)
        {
            (*counter_map_)[tag].logOnWindowReached<unit>(tag);
        }
    }

    template <Unit unit>
    inline TimeCounterScope<unit>::TimeCounterScope(std::string codeblock_tag)
    {
        this->init(codeblock_tag);
    }

    template <Unit unit>
    inline void TimeCounterScope<unit>::reset()
    {
        this->deinit();
    }
    template <Unit unit>
    inline void TimeCounterScope<unit>::reset(std::string codeblock_tag)
    {
        this->deinit();
        this->init(codeblock_tag);
    }

    template <Unit unit>
    inline TimeCounterScope<unit>::TimeCounterScope(TimeCounterScope<unit>&& obj) noexcept
        : valid_(obj.valid_), start_t_(obj.start_t_), codeblock_tag_(std::move(obj.codeblock_tag_))
    {
        obj.valid_ = false;
    }

    template <Unit unit>
    inline TimeCounterScope<unit>::~TimeCounterScope()
    {
        this->deinit();
    }

    template <Unit unit>
    inline void TimeCounterScope<unit>::init(std::string& codeblock_tag)
    {
        start_t_ = std::chrono::steady_clock::now();
        codeblock_tag_ = std::move(codeblock_tag);
        valid_ = true;
    }

    template <Unit unit>
    inline void TimeCounterScope<unit>::deinit()
    {
        if (valid_ && time::_getGlobalTimeCounterEnabled().load(std::memory_order_relaxed))
        {
            Rep time_cost;
            const char* msg = nullptr;
            switch (unit)
            {
            case time::SEC:
                time_cost = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_t_).count();
                msg = "TimeCounterScope: {} cost time {}s";
                break;
            case time::MSEC:
                time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_t_).count();
                msg = "TimeCounterScope: {} cost time {}ms";
                break;
            case time::USEC:
                time_cost = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_t_).count();
                msg = "TimeCounterScope: {} cost time {}us";
                break;
            case time::NSEC:
                time_cost = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_t_).count();
                msg = "TimeCounterScope: {} cost time {}ns";
                break;
            default:
                time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_t_).count();
                msg = "TimeCounterScope: {} cost time {}ms";
                break;
            }
            MEIDO_INFO_RAW(msg, codeblock_tag_, time_cost);
        }
        valid_ = false;
    }

    template <Unit unit>
    inline MinTimeScope<unit>::MinTimeScope(Rep duration, Rep sleep_interval, std::function<bool()> quit_pred)
    {
        this->init(duration, sleep_interval, std::move(quit_pred));
    }

    template <Unit unit>
    inline MinTimeScope<unit>::~MinTimeScope()
    {
        this->deinit();
    }

    template <Unit unit>
    inline void MinTimeScope<unit>::reset()
    {
        this->deinit();
    }

    template <Unit unit>
    inline void MinTimeScope<unit>::reset(Rep duration, Rep sleep_interval, std::function<bool()> quit_pred)
    {
        this->deinit();
        this->init(duration, sleep_interval, std::move(quit_pred));
    }

    template <Unit unit>
    inline MinTimeScope<unit>::MinTimeScope(MinTimeScope<unit>&& obj) noexcept :
        valid_(obj.valid_),
        sleep_interval_(obj.sleep_interval_),
        target_tp_(obj.target_tp_),
        quit_pred_(std::move(obj.quit_pred_))
    {
        obj.valid_ = false;
    }

    template <Unit unit>
    inline void MinTimeScope<unit>::init(Rep duration, Rep sleep_interval, std::function<bool()> quit_pred)
    {
        auto now_tp = std::chrono::steady_clock::now();
        auto now_timeval = std::chrono::duration_cast<std::chrono::nanoseconds>(now_tp.time_since_epoch()).count();
        if (duration < std::chrono::nanoseconds::min().count() / unit)
        {
            duration = std::chrono::nanoseconds::min().count();
            MEIDO_WARN("Param duration too small, set to min supported value");
        }
        else if (duration > std::chrono::nanoseconds::max().count() / unit)
        {
            duration = std::chrono::nanoseconds::max().count();
            MEIDO_WARN("Param duration too large, set to max supported value");
        }
        else
            duration = duration * unit;
        Rep target_tp_val = _priv::safeAdd(now_timeval, duration, MEIDO_FUNCNAME);    // 防止溢出
        target_tp_ = std::chrono::steady_clock::time_point(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::nanoseconds(target_tp_val)));
        if (sleep_interval <= 0)
            sleep_interval_ = time::MSEC;
        else if (sleep_interval > duration / unit)
            sleep_interval_ = duration;
        else
            sleep_interval_ = sleep_interval * unit;
        if (!quit_pred)
            quit_pred = []() { return false; };
        quit_pred_ = std::move(quit_pred);
        valid_ = true;
    }

    template <Unit unit>
    inline void MinTimeScope<unit>::deinit()
    {
        if (valid_)
        {
            Rep sleep_interval_shorter = sleep_interval_ / 10 > 0 ? sleep_interval_ / 10 : 1;
            // 先睡大块时间，减少系统调用开销，再精确等待剩余时间
            while (!quit_pred_())
            {
                auto remain_duration = target_tp_ - std::chrono::steady_clock::now();
                if (remain_duration > std::chrono::nanoseconds(sleep_interval_))
                {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_interval_));
                }
                else if (remain_duration > std::chrono::nanoseconds(0))
                {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_interval_shorter));
                }
                else
                {
                    break;
                }
            }
        }
        valid_ = false;
        quit_pred_ = {};
        target_tp_ = std::chrono::steady_clock::time_point{};
    }

}    // namespace time
}    // namespace meido

#endif    // !TIME_HPP_BOKUMEIDOCPP
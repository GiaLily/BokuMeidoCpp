/*  bokumeidocpp库的日志模块，可以指定将日志(包括库内日志)输出到文件、标准输出或用户自定义函数  */
#pragma once
#ifndef LOG_HPP_BOKUMEIDOCPP
#define LOG_HPP_BOKUMEIDOCPP

#include <chrono>
#include <string>

#include "base.hpp"
#include "str.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

/*
    日志模块的用户接口
    - 无操作时日志将默认打印至stdout/stderr
    - setup后，日志根据配置输出至指定目标
    - reset后，将flush现有日志，并退化为默认状态
    警告：日志模块的接口不建议在信号中断处理函数中调用，因为中断可能在任意时刻发生，即使是锁内
*/


/*  将fmt_str格式化后的字符串输出到日志，每次调用会自动换行，线程安全
    - 基于meido::str::format格式化字符串，可将任意类型参数转换为字符串
    - 支持的类型将正常转换为字符串，不支持的类型将转为<ClassName: Address>形式的字符串，详见meido::str::format
    - fmt_str接收C字符串，格式如"Hello {}! Only {} world"
    - 用例：MEIDO_LOG_I("Hello {}! Only {} world", "world", 1)  */
#define MEIDO_DEBUG(fmt_str, ...) MEIDO_LOG(meido::log::Level::DEBUG, fmt_str, ##__VA_ARGS__)
#define MEIDO_INFO(fmt_str, ...) MEIDO_LOG(meido::log::Level::INFO, fmt_str, ##__VA_ARGS__)
#define MEIDO_WARN(fmt_str, ...) MEIDO_LOG(meido::log::Level::WARN, fmt_str, ##__VA_ARGS__)
#define MEIDO_ERROR(fmt_str, ...) MEIDO_LOG(meido::log::Level::ERROR, fmt_str, ##__VA_ARGS__)

// 以下是RAW类型日志，与非RAW类型日志对应级别同级，RAW类型日志输出中不包含头信息，仅包含格式化后的字符串
#define MEIDO_DEBUG_RAW(fmt_str, ...) MEIDO_LOG_RAW(meido::log::Level::DEBUG, fmt_str, ##__VA_ARGS__)
#define MEIDO_INFO_RAW(fmt_str, ...) MEIDO_LOG_RAW(meido::log::Level::INFO, fmt_str, ##__VA_ARGS__)
#define MEIDO_WARN_RAW(fmt_str, ...) MEIDO_LOG_RAW(meido::log::Level::WARN, fmt_str, ##__VA_ARGS__)
#define MEIDO_ERROR_RAW(fmt_str, ...) MEIDO_LOG_RAW(meido::log::Level::ERROR, fmt_str, ##__VA_ARGS__)

#define MEIDO_LOG(level, fmt_str, ...) \
    meido::_priv::log(level, false, __FILE__, __LINE__, fmt_str, ##__VA_ARGS__)
#define MEIDO_LOG_RAW(level, fmt_str, ...) \
    meido::_priv::log(level, true, __FILE__, __LINE__, fmt_str, ##__VA_ARGS__)

namespace log
{
    // 若未加载日志系统或加载失败，日志会默认同步打印至stdout/stderr

    //  设置终端日志输出级别
    int setupConsoleLogger(log::Level min_level);

    /*  加载同步日志系统，指定将日志(包括库内日志)输出到文件。日志加载函数只能被成功调用一次
        @param min_level: 日志级别，低于该级别的日志将被忽略，RAW类型与非RAW类型日志对应级别同级
        @param log_dir: 日志目录
        @param name_prefix: 项目名
        @param rotation_policy: 日志滚动策略
        @return 0表示成功，其他表示失败 */
    int setupSyncLogger(log::Level min_level, std::string log_dir, std::string name_prefix, RotationPolicy rotation_policy = {});


    /*  加载同步日志系统，指定将日志(包括库内日志)输出到用户自定义回调函数。日志加载函数只能被成功调用一次
        @param min_level: 日志级别，低于该级别的日志将被忽略，RAW类型与非RAW类型日志对应级别同级
        @param user_func: 用户自定义函数，将日志消息将直接传递给该回调，不再做其他输出。回调函数会被并行调用，线程安全由用户保证
        @return 0表示成功，其他表示失败 */
    int setupSyncLogger(log::Level min_level, void (*user_func)(log::Message));

    /*  加载异步日志系统，指定将日志(包括库内日志)输出到文件。日志加载函数只能被成功调用一次
        @param min_level: 日志级别，低于该级别的日志将被忽略，RAW类型与非RAW类型日志对应级别同级
        @param log_dir: 日志目录
        @param name_prefix: 项目名
        @param rotation_policy: 日志滚动策略
        @param capacity: 日志队列最大容纳数量
        @return 0表示成功，其他表示失败 */
    int setupAsyncLogger(log::Level min_level, std::string log_dir, std::string name_prefix, RotationPolicy rotation_policy = {}, size_t capacity = 1024);

    // 卸载日志系统，flush之前所有日志，之后的日志将退化为默认打印至stdout/stderr
    void resetLogger();



}    // namespace log

namespace _priv
{
    template <class... Args>
    inline void log(log::Level level, bool is_raw, const char* filename, int line, const char* fmt, const Args&... args)
    {
        // 早期过滤：在格式化之前检查级别，避免 str::format 的不必要开销
        if (level < _priv::g_base_logger->getMinLevel())
            return;
        std::chrono::system_clock::time_point now_t = std::chrono::system_clock::now();
        std::string msg = str::format(fmt, args...);
        _priv::g_base_logger->log(level, is_raw, filename, line, now_t, std::move(msg));
    }
}    // namespace _priv










/*--------------------------------------------内部实现--------------------------------------------*/

namespace log
{
    inline int setupConsoleLogger(log::Level min_level)
    {
        return _priv::g_base_logger->setupConsole(min_level);
    }

    inline int setupSyncLogger(log::Level min_level, std::string log_dir, std::string name_prefix, RotationPolicy rotation_policy)
    {
        return _priv::g_base_logger->setupSync(min_level, std::move(log_dir), std::move(name_prefix), rotation_policy);
    }

    inline int setupSyncLogger(log::Level min_level, void (*user_func)(log::Message))
    {
        return _priv::g_base_logger->setupSync(min_level, user_func);
    }

    inline int setupAsyncLogger(log::Level min_level, std::string log_dir, std::string name_prefix, RotationPolicy rotation_policy, size_t capacity)
    {
        return _priv::g_base_logger->setupAsync(min_level, std::move(log_dir), std::move(name_prefix), rotation_policy, capacity);
    }

    inline void resetLogger()
    {
        _priv::g_base_logger->reset();
    }
}    // namespace log


}    // namespace meido




#endif
/*  log 模块性能测试
    测试内容：
    - 日志写入吞吐量（使用用户回调，避免 IO 噪音）
    - 不同级别日志的过滤开销
    - RAW 日志与普通日志的开销对比
    - 组件拆解：format / 时间戳 / 原子操作各自开销
*/
#pragma once
#ifndef BENCH_LOG_HPP
#define BENCH_LOG_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <vector>
#include <string>
#include <atomic>

#ifdef HAS_GLOG
#include <glog/logging.h>    // glog 对比测试
#endif

using namespace meido;

// 跨平台 localtime 封装
inline tm localtimePortable(const time_t* t)
{
    tm buf;
#ifdef _MSC_VER
    localtime_s(&buf, t);
#else
    localtime_r(t, &buf);
#endif
    return buf;
}

namespace _meidologbench
{

// 无操作的日志回调：只计数，不输出
static std::atomic<int64_t> g_log_count{0};
static void nullSink(log::Message)
{
    g_log_count.fetch_add(1, std::memory_order_relaxed);
}

inline void check()
{
    BENCH_MODULE("log 模块");

    log::resetLogger();
    log::setupSyncLogger(log::Level::INFO, nullSink);

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. MEIDO_INFO 日志基本开销 ----
    {
        g_log_count.store(0);
        BENCH_N("MEIDO_INFO(hello world) 纯日志", {
            MEIDO_INFO("hello world");
        });
    }
    BENCH_SEP();

    // ---- 2. MEIDO_DEBUG (低于 min_level，被过滤) ----
    {
        g_log_count.store(0);
        BENCH_N("MEIDO_DEBUG (低于级别被过滤)", {
            MEIDO_DEBUG("this should be filtered");
        });
    }
    BENCH_SEP();

    // ---- 3. 带参数的日志 ----
    {
        int a = 42;
        double b = 3.14;
        g_log_count.store(0);
        BENCH_N("MEIDO_INFO({} {}, a, b) 带参数", {
            MEIDO_INFO("int={}, double={}", a, b);
        });
    }
    BENCH_SEP();

    // ---- 4. RAW 日志（无头信息） ----
    {
        g_log_count.store(0);
        BENCH_N("MEIDO_INFO_RAW 无头信息", {
            MEIDO_INFO_RAW("hello world");
        });
    }
    BENCH_SEP();

    // ---- 5. MEIDO_WARN / MEIDO_ERROR 各级别 ----
    {
        g_log_count.store(0);
        BENCH_N("MEIDO_WARN 级别", {
            MEIDO_WARN("this is a warning message for testing");
        });
        BENCH_N("MEIDO_ERROR 级别", {
            MEIDO_ERROR("this is an error message for testing");
        });
    }
    BENCH_SEP();

    // ================================================================
    // 组件拆解
    // ================================================================
    {
        BENCH_MODULE("log 组件拆解 (同步模式)");

        // ---- 组件A: 纯 format 无占位符 ----
        {
            BENCH_N("[组件] str::format(hello world) 纯格式化", {
                volatile auto s = str::format("hello world");
                (void)s;
            });
        }

        // ---- 组件B: 时间戳管道 ----
        {
            BENCH_N("[组件] 时间戳管道 (now+to_time_t+localtime)", {
                auto now_t = std::chrono::system_clock::now();
                time_t now_time_t = std::chrono::system_clock::to_time_t(now_t);
                volatile auto buf = localtimePortable(&now_time_t);
                volatile int usec = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        now_t.time_since_epoch()).count() % 1000);
                (void)buf; (void)usec;
            });
        }

        // ---- 组件D: 原子操作 ----
        {
            std::atomic<int64_t> ctr{0};
            BENCH_N("[组件] 3x atomic fetch_add (relaxed)", {
                ctr.fetch_add(1, std::memory_order_relaxed);
                ctr.fetch_add(1, std::memory_order_relaxed);
                ctr.fetch_sub(1, std::memory_order_relaxed);
            });
        }

        BENCH_SEP();
        std::cout << "[SUMMARY] MEIDO_INFO = format + 时间戳 + ThreadId + 3x原子 + 回调" << std::endl;
        std::cout << "  MEIDO_INFO_RAW = format + 3x原子" << std::endl;
        BENCH_SEP();
    }

    // ================================================================
    // BokuMeidoCpp 文件输出 & 同步 vs 异步
    // ================================================================
    {
        BENCH_MODULE("log 文件输出 & 同步/异步");

        // 跨平台临时目录
#ifdef _MSC_VER
        const char* tmpDir = ".";
#else
        const char* tmpDir = "/tmp";
#endif

        // ---- 同步文件模式 ----
        {
            std::string path = std::string(tmpDir) + "/bokumeido_sync";
            log::resetLogger();
            log::setupSyncLogger(log::Level::INFO, path, "test");
            {
                BENCH_N("[sync-file]  INFO 纯消息", {
                    MEIDO_INFO("hello world");
                });
            }
            {
                int a = 42;
                double b = 3.14;
                BENCH_N("[sync-file]  INFO 带参数", {
                    MEIDO_INFO("int={}, double={}", a, b);
                });
            }
        }
        BENCH_SEP();

        // ---- 异步文件模式 ----
        {
            std::string path = std::string(tmpDir) + "/bokumeido_async";
            log::resetLogger();
            log::setupAsyncLogger(log::Level::INFO, path, "test", log::RotationPolicy{}, 1024);
            {
                BENCH_N("[async-file] INFO 纯消息", {
                    MEIDO_INFO("hello world");
                });
            }
            {
                int a = 42;
                double b = 3.14;
                BENCH_N("[async-file] INFO 带参数", {
                    MEIDO_INFO("int={}, double={}", a, b);
                });
            }
        }
        BENCH_SEP();
        std::cout << "[SUMMARY] 同步=调用线程写文件  异步=调用线程推队列+后台线程写文件" << std::endl;
        BENCH_SEP();
    }

    // ================================================================
    // BokuMeidoCpp vs glog 对比 (需安装 glog)
    // ================================================================
#ifdef HAS_GLOG
    {
        BENCH_MODULE("log BokuMeidoCpp vs glog");

        static bool glog_inited = false;
        if (!glog_inited)
        {
            google::InitGoogleLogging("bokumeido_bench");
            glog_inited = true;
        }

        // ---- 模式 A: 无IO ----
        {
            BENCH_MODULE("  [模式A] 无IO — BokuMeidoCpp回调 vs glog /dev/null");

            log::setupSyncLogger(log::Level::INFO, nullSink);
            {
                g_log_count.store(0);
                BENCH_N("[BokuMeidoCpp-cb] INFO 纯消息", {
                    MEIDO_INFO("hello world");
                });
            }
            {
                int a = 42;
                double b = 3.14;
                g_log_count.store(0);
                BENCH_N("[BokuMeidoCpp-cb] INFO 带参数", {
                    MEIDO_INFO("int={}, double={}", a, b);
                });
            }

            FLAGS_logtostderr = false;
            google::SetLogDestination(google::GLOG_INFO, "/dev/null");
            google::SetLogDestination(google::GLOG_WARNING, "/dev/null");
            google::SetLogDestination(google::GLOG_ERROR, "/dev/null");
            {
                BENCH_N("[glog-null]    INFO 纯消息", {
                    LOG(INFO) << "hello world";
                });
            }
            {
                int a = 42;
                double b = 3.14;
                BENCH_N("[glog-null]    INFO 带参数", {
                    LOG(INFO) << "int=" << a << ", double=" << b;
                });
            }
            BENCH_SEP();
        }

        // ---- 模式 B: 真实文件 ----
        {
            BENCH_MODULE("  [模式B] 文件输出 — 双方均写入真实文件");

            log::resetLogger();
            log::setupSyncLogger(log::Level::INFO, "/tmp/bokumeido_bench", "test");
            {
                BENCH_N("[BokuMeidoCpp-file] INFO 纯消息", {
                    MEIDO_INFO("hello world");
                });
            }
            {
                int a = 42;
                double b = 3.14;
                BENCH_N("[BokuMeidoCpp-file] INFO 带参数", {
                    MEIDO_INFO("int={}, double={}", a, b);
                });
            }

            FLAGS_log_dir = "/tmp/glog_bench";
            FLAGS_logtostderr = false;
            google::SetLogDestination(google::GLOG_INFO, "");
            {
                BENCH_N("[glog-file]     INFO 纯消息", {
                    LOG(INFO) << "hello world";
                });
            }
            {
                int a = 42;
                double b = 3.14;
                BENCH_N("[glog-file]     INFO 带参数", {
                    LOG(INFO) << "int=" << a << ", double=" << b;
                });
            }
            BENCH_SEP();
        }

        log::resetLogger();
        google::ShutdownGoogleLogging();
        glog_inited = false;

        BENCH_SEP();
        std::cout << "[SUMMARY] 模式A=无IO 模式B=文件写入(glog对比)" << std::endl;
        BENCH_SEP();
    }
#else
    {
        BENCH_MODULE("log BokuMeidoCpp vs glog [跳过: 未安装 glog]");
    }
#endif

    // 恢复到 benchmark 默认状态（同步+回调）
    log::resetLogger();
    log::setupSyncLogger(log::Level::INFO, nullSink);
    BENCH_SEP();
}

} // namespace _meidologbench

#endif // BENCH_LOG_HPP

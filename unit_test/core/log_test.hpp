/*  log 模块单元测试
    分类：级别过滤 | 格式化输出 | RAW标志 | 重复setup检测  */
#pragma once
#ifndef LOG_TEST_HPP_BOKUMEIDOCPP
#define LOG_TEST_HPP_BOKUMEIDOCPP

#include <mutex>
#include <vector>

#include "bokumeido/core/log.hpp"

using namespace meido;
namespace _meidologcheck
{

static std::vector<log::Message> g_logs;
static std::mutex g_log_mtx;

static void logCallback(log::Message msg)
{
    std::lock_guard<std::mutex> lk(g_log_mtx);
    g_logs.push_back(std::move(msg));
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check log start--------------------");

    // === 所有测试合并到单个函数中，避免多次 reset/setup ===
    g_logs.clear();

    // ---- 1. 级别过滤 ----
    {
        int ret = log::setupSyncLogger(log::Level::WARN, logCallback);
        if (ret != 0)
        {
            // 首次 setup 可能因其他模块早期日志操作而失败
            log::resetLogger();
            ret = log::setupSyncLogger(log::Level::WARN, logCallback);
        }
        MEIDO_ASSERT(ret == 0);

        MEIDO_INFO("should be filtered");
        MEIDO_WARN("should pass warning {}", 1);
        MEIDO_ERROR("should pass error");

        MEIDO_ASSERT(g_logs.size() == 2);
        MEIDO_ASSERT(g_logs[0].level == log::Level::WARN);
        MEIDO_ASSERT(g_logs[0].msg == "should pass warning 1");
        MEIDO_ASSERT(g_logs[1].level == log::Level::ERROR);
        MEIDO_ASSERT(g_logs[1].msg == "should pass error");
    }

    // ---- 2. 格式化 ----
    {
        log::resetLogger();
        g_logs.clear();
        MEIDO_ASSERT(log::setupSyncLogger(log::Level::DEBUG, logCallback) == 0);

        MEIDO_DEBUG("int {}", 42);
        MEIDO_INFO("str {}", std::string("hello"));
        MEIDO_WARN("multi {} {}", 1, 2);

        MEIDO_ASSERT(g_logs.size() == 3);
        MEIDO_ASSERT(g_logs[0].msg == "int 42");
        MEIDO_ASSERT(g_logs[1].msg == "str hello");
        MEIDO_ASSERT(g_logs[2].msg == "multi 1 2");
    }

    // ---- 3. RAW 标志 ----
    {
        log::resetLogger();
        g_logs.clear();
        MEIDO_ASSERT(log::setupSyncLogger(log::Level::DEBUG, logCallback) == 0);

        MEIDO_INFO_RAW("raw message");
        MEIDO_INFO("normal message");

        MEIDO_ASSERT(g_logs.size() == 2);
        MEIDO_ASSERT(g_logs[0].is_raw == true);
        MEIDO_ASSERT(g_logs[0].msg == "raw message");
        MEIDO_ASSERT(g_logs[1].is_raw == false);
        MEIDO_ASSERT(g_logs[1].msg == "normal message");
    }

    // ---- 4. 重复 setup 检测 ----
    {
        log::resetLogger();
        g_logs.clear();
        MEIDO_ASSERT(log::setupSyncLogger(log::Level::DEBUG, logCallback) == 0);
        printf("Author check! Expected| [WARN] Logger has been initialized, ignore initSync call (duplicate setup test)\n");
        MEIDO_ASSERT(log::setupSyncLogger(log::Level::DEBUG, logCallback) != 0);
    }

    log::resetLogger();
    _MEIDO_INFO_RAW("---------------------check log end---------------------\n\n");
}

}    // namespace _meidologcheck

#endif    // !LOG_TEST_HPP_BOKUMEIDOCPP

/*  io 模块性能测试
    测试内容：
    - print() 多参数输出（重定向到 /dev/null 避免屏幕 IO）
    - ArgumentParser 解析速度
    - BooleanOption / ValueOption 构造

    注意：ArgumentParser::parse 内部会输出日志，
    因此在测试前切换到无操作回调以抑制日志输出。
*/
#pragma once
#ifndef BENCH_IO_HPP
#define BENCH_IO_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <vector>
#include <string>
#include <sstream>

using namespace meido;

namespace _meidoiobench
{

// 丢弃型重定向：cout 输出直接丢弃（不累积内存），宏结果经 bench::out() 走 stderr 不被吞
inline std::streambuf* _beginNullRedirect()
{
    return bench::redirectCout();
}

inline void _endNullRedirect(std::streambuf* old_buf)
{
    bench::restoreCout(old_buf);
}

inline void check()
{
    BENCH_MODULE("io 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. print 基本类型 ----
    // 重定向在循环外完成一次，测的是 print 真实开销（不含 rdbuf 切换）
    {
        bench::setOut(std::cerr);           // 宏结果改道 stderr，避免被 cout 重定向吞掉
        std::streambuf* old_buf = _beginNullRedirect();
        BENCH_N("print(42) 打印整数", {
            io::print(42);
        });
        _endNullRedirect(old_buf);
        bench::setOut(std::cout);
    }
    BENCH_SEP();

    // ---- 2. print 多参数 ----
    {
        bench::setOut(std::cerr);
        std::streambuf* old_buf = _beginNullRedirect();
        BENCH_N("print(\"str\", 42, 3.14) 多参数", {
            io::print("hello", 42, 3.14);
        });
        _endNullRedirect(old_buf);
        bench::setOut(std::cout);
    }
    BENCH_SEP();

    // ---- 3. print 容器 ----
    {
        std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        bench::setOut(std::cerr);
        std::streambuf* old_buf = _beginNullRedirect();
        BENCH_N("print(vector<int>) 10元素容器", {
            io::print(vec);
        });
        _endNullRedirect(old_buf);
        bench::setOut(std::cout);
    }
    BENCH_SEP();

    // ---- 4. ArgumentParser 构造 ----
    {
        BENCH_N_CONSTEXPR("ArgumentParser 构造", {
            io::ArgumentParser parser;
            (void)parser;
        });
    }
    BENCH_SEP();

    // ---- 5. ArgumentParser 解析 ----
    // 注：parse 内部会输出 INFO 日志，先将日志级别提到 ERROR 抑制输出
    {
        // 切换到无操作回调日志抑制 parse 内部日志
        log::resetLogger();
        log::setupSyncLogger(log::Level::ERROR,
            [](log::Message) {});
        const char* args[] = {"program", "-b", "--long", "-v", "value", "extra"};
        // 需要用非常量 argv
        char* argv[] = {const_cast<char*>("program"), const_cast<char*>("-b"),
                        const_cast<char*>("--long"), const_cast<char*>("-v"),
                        const_cast<char*>("value")};
        std::vector<io::BooleanOption> bool_opts = {
            {"-b", "--bool", "a boolean flag"},
            {"", "--long", "another boolean flag"},
        };
        std::vector<io::ValueOption> val_opts = {
            {"-v", "--value", "a value option", ""},
        };

        io::ArgumentParser parser;
        BENCH_N("ArgumentParser::parse (5 args, 3 presets)", {
            parser.parse(5, argv, bool_opts, val_opts);
        });

        // 恢复日志
        log::resetLogger();
    }
    BENCH_SEP();

    // ---- 6. BooleanOption / ValueOption 构造 ----
    {
        BENCH_N_CONSTEXPR("BooleanOption(\"-s\", \"--short\", \"desc\") 构造", {
            io::BooleanOption opt = {"-s", "--short", "description"};
            (void)opt;
        });
        BENCH_N_CONSTEXPR("ValueOption(\"-v\", \"--value\", \"desc\", \"def\") 构造", {
            io::ValueOption opt = {"-v", "--value", "description", "default"};
            (void)opt;
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidoiobench

#endif // BENCH_IO_HPP
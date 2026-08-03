/*  path 模块性能测试
    测试内容：
    - normPath 路径标准化
    - splitName / splitExt 路径解析
    - join 路径拼接
    - exists / isDir / isFile 文件系统查询
    - walk 目录遍历
*/
#pragma once
#ifndef BENCH_PATH_HPP
#define BENCH_PATH_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <string>

using namespace meido;

namespace _meidopathbench
{

inline void check()
{
    BENCH_MODULE("path 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. normPath ----
    {
        std::string win_path = "C:\\\\Users\\\\test\\\\..\\\\work\\\\file.txt";
        BENCH_N("normPath(C:\\Users\\..\\work\\file.txt)", {
            volatile auto r = path::normPath(win_path);
            (void)r;
        });
    }
    BENCH_SEP();

    {
        std::string unix_path = "/home/user/./work/../project/src/file.cpp";
        BENCH_N("normPath(/home/user/./work/../src/file.cpp)", {
            volatile auto r = path::normPath(unix_path);
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 2. splitName / splitExt ----
    {
        std::string p = "/home/user/project/src/file.cpp";
        BENCH_N("splitName(p) 获取文件名", {
            volatile auto r = path::splitName(p);
            (void)r;
        });
        BENCH_N("splitExt(p) 获取扩展名", {
            volatile auto r = path::splitExt(p);
            (void)r;
        });
        BENCH_N("splitName(p, false) 不含扩展名", {
            volatile auto r = path::splitName(p, false);
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 3. join ----
    {
        BENCH_N("join(\"/home\", \"user\", \"file.txt\") 3段", {
            volatile auto r = path::join("/home", "user", "file.txt");
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 4. exists / isDir / isFile（缓存命中，同一路径） ----
    {
        std::string p = "..";
        BENCH_N("isDir(\"..\") (stat 系统调用)", {
            volatile bool r = path::isDir(p);
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 5. parent ----
    {
        std::string p = "/home/user/project/src/file.cpp";
        BENCH_N("parent(\"/home/.../file.cpp\")", {
            volatile auto r = path::parent(p);
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 6. isAbs ----
    {
        BENCH_N("isAbs(\"/home/user\") 绝对路径判断", {
            volatile bool r = path::isAbs("/home/user");
            (void)r;
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidopathbench

#endif // BENCH_PATH_HPP

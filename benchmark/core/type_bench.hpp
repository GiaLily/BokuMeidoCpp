/*  type 模块性能测试
    测试内容：
    - getTypeName<T>() 对不同类型调用的开销（RTTI + demangle）
    - 编译期类型特征检查器（均为编译期零成本，仅验证）
*/
#pragma once
#ifndef BENCH_TYPE_HPP
#define BENCH_TYPE_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <string>
#include <vector>
#include <map>

using namespace meido;

namespace _meidotypebench
{

struct _MyStruct
{
    int x;
    double y;
    std::string z;
};

template <class T>
class _MyTemplate {};

inline void check()
{
    BENCH_MODULE("type 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. getTypeName<int>() ----
    // 设计意图：最基本的整数类型 demangle，涉及 typeid + __cxa_demangle + free
    // 预期开销最小（int 在 Itanium ABI 中名为 "i"，demangle 极快）
    {
        BENCH_N("getTypeName<int>()", {
            volatile auto s = type::getTypeName<int>();
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 2. getTypeName<std::string>() ----
    // 设计意图：STL 类型名较长（"std::__cxx11::basic_string<...>"），
    // demangle 和 string 构造开销比 int 大
    {
        BENCH_N("getTypeName<std::string>()", {
            volatile auto s = type::getTypeName<std::string>();
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 3. getTypeName<_MyStruct>() ----
    // 设计意图：自定义结构体，名字中不含模板参数，demangle 较简单
    {
        BENCH_N("getTypeName<_MyStruct>()", {
            volatile auto s = type::getTypeName<_MyStruct>();
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 4. getTypeName<_MyTemplate<_MyStruct>>() ----
    // 设计意图：模板嵌套类型，demangle 需要递归解析模板参数
    {
        BENCH_N("getTypeName<_MyTemplate<_MyStruct>>()", {
            volatile auto s = type::getTypeName<_MyTemplate<_MyStruct>>();
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 5. getTypeName<map<string,vector<int>>>() ----
    // 设计意图：深层嵌套的 STL 容器类型，demangle 名极长，
    // 涉及多次 malloc/free 和 string 拷贝
    {
        using ComplexType = std::map<std::string, std::vector<int>>;
        BENCH_N("getTypeName<map<string,vector<int>>>()", {
            volatile auto s = type::getTypeName<ComplexType>();
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 6. 编译期检查器（编译期常量，应被优化为 0ms） ----
    // 设计意图：验证 EachTrueChecker / InTypesChecker 等模板元工具
    // 在运行时的确零成本。理论上编译器会将其完全优化。
    {
        BENCH_N_CONSTEXPR("EachTrueChecker (编译期常量)", {
            volatile bool v = type::EachTrueChecker<true, true, true>::value;
            (void)v;
        });
        BENCH_N_CONSTEXPR("InTypesChecker (编译期常量)", {
            volatile bool v = type::InTypesChecker<int, float, double, int, char>::value;
            (void)v;
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidotypebench

#endif // BENCH_TYPE_HPP

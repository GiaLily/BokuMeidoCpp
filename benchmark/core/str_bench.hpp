/*  str 模块性能测试
    测试内容：
    - toStr<T>() 不同类型转换
    - format() 字符串格式化
    - split / rsplit / splitAny / rsplitAny 分割性能
    - trim / lpad / rpad 字符串变换
    - ordinalize
*/
#pragma once
#ifndef BENCH_STR_HPP
#define BENCH_STR_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <vector>
#include <string>
#include <map>

using namespace meido;

namespace _meidostrbench
{

inline void check()
{
    BENCH_MODULE("str 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ======================================================================
    // toStr 系列 — 验证不同类型通过 ostringstream 转换为 string 的开销
    // 内部使用 thread_local ostringstream，每次 str() 触发 string 拷贝
    // ======================================================================

    // ---- 1. toStr<int> ----
    // 设计意图：整数转字符串，ostringstream << int 是最常用的格式化路径
    {
        int val = 123456789;
        BENCH_N("toStr<int>(123456789)", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 2. toStr<double> (短浮点，SSO内) ----
    // 设计意图：输出 "1.23" (4字符)，验证SSO级浮点转换的开销（无堆分配）
    {
        double val = 1.23;
        BENCH_N("toStr<double>(1.23) 短浮点", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 3. toStr<double> (长浮点，超SSO) ----
    // 设计意图：输出约16字符，超MSVC SSO(15)触发堆分配，对比短浮点可验证堆开销
    {
        double val = 3.14159265358979;
        BENCH_N("toStr<double>(pi) 长浮点", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 4. toStr<string> (ConstructibleFromEachChecker 分支) ----
    // 设计意图：已有 string 直接返回拷贝，无 ostringstream 参与
    {
        std::string val = "hello world hello world hello world";
        BENCH_N("toStr<string>(已有字符串)", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 4. toStr<const char*> ----
    // 设计意图：C 字符串通过 ostream::operator<<(const char*) 输出
    {
        const char* val = "hello world hello world hello world";
        BENCH_N("toStr<const char*>(C字符串)", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 5. toStr<vector<int>> ----
    // 设计意图：STL 容器递归遍历输出，每次元素调用 osInput
    {
        std::vector<int> val = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        BENCH_N("toStr<vector<int>>(10元素)", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 6. toStr<map<string,int>> ----
    // 设计意图：map 遍历涉及 pair 递归 + string key 的 osInput
    {
        std::map<std::string, int> val = {
            {"one", 1}, {"two", 2}, {"three", 3},
            {"four", 4}, {"five", 5}, {"six", 6}};
        BENCH_N("toStr<map<string,int>>(6元素)", {
            volatile auto s = str::toStr(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 7. toStr 精度控制 ----
    // 设计意图：setprecision 影响浮点格式化路径
    {
        double val = 3.14159265358979;
        BENCH_N("toStr<3>(pi) 精度控制", {
            volatile auto s = str::toStr<3>(val);
            (void)s;
        });
    }
    BENCH_SEP();

    // ======================================================================
    // format 系列 — 验证 str::format 在不同参数类型下的格式化开销
    // 内部使用 strstr 查找 "{}"，逐段 osInput 拼接
    // 对比不同类型参数的格式化性能差异：纯整型、纯浮点型、纯字符串型、混合型
    // ======================================================================

    // ---- 9. format 纯整型参数 ----
    {
        int a = 42, b = 123, c = -7;
        BENCH_N("format(\"int {} {} {}\") 3纯整型", {
            volatile auto s = str::format("int {} {} {}", a, b, c);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 10. format 纯短浮点型参数 ----
    // 设计意图：短浮点 "1.23" 等，DragonBox 只需处理少量数字
    {
        double a = 3.14, b = 2.718, c = 1.5;
        BENCH_N("format(\"float {} {} {}\") 3纯短浮点型", {
            volatile auto s = str::format("float {} {} {}", a, b, c);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 11. format 纯长浮点型参数 ----
    // 设计意图：长浮点如 pi，DragonBox 需处理十余位有效数字，输出字符串更长
    {
        double a = 3.14159265358979, b = 2.71828182845904, c = 1.41421356237309;
        BENCH_N("format(\"float {} {} {}\") 3纯长浮点型", {
            volatile auto s = str::format("float {} {} {}", a, b, c);
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 12. format 纯字符串型参数 ----
    {
        BENCH_N("format(\"str {} {} {}\") 3纯字符串型", {
            volatile auto s = str::format("str {} {} {}", "hello", "world", "test");
            (void)s;
        });
    }
    BENCH_SEP();

    // ---- 13. format 混合型参数 ----
    // 设计意图：int + float + string 混合，模拟真实日志场景
    {
        int a = 42;
        double b = 3.14;
        std::string c = "test";
        BENCH_N("format(\"int={} float={} str={}\") 混合型", {
            volatile auto s = str::format("int={} float={} str={}", a, b, c);
            (void)s;
        });
    }
    BENCH_SEP();

    // ======================================================================
    // split 系列 — 验证字符串分割的性能
    // ======================================================================

    // ---- 10. split 单字符分隔符 10段 ----
    // 设计意图：string::find 单字符搜索，substr 截取，vector 填充
    {
        std::string s = "a,b,c,d,e,f,g,h,i,j";
        BENCH_N("split(\"a,b,c,...\", \",\") 10段", {
            volatile auto v = str::split(s, ",");
            (void)v;
        });
    }
    BENCH_SEP();

    // ---- 11. split 多字符分隔符 ----
    // 设计意图：find 多字符搜索，每次 substring 操作更复杂
    {
        std::string s = "a::b::c::d::e::f::g::h::i::j";
        BENCH_N("split(\"a::b::c...\", \"::\") 多字符分隔符", {
            volatile auto v = str::split(s, "::");
            (void)v;
        });
    }
    BENCH_SEP();

    // ---- 12. split 100段CSV（长字符串）----
    // 设计意图：大量分段触发 vector 多次扩容+string 拷贝，内存压力最大
    {
        std::string s;
        for (int i = 0; i < 100; i++)
            s += std::to_string(i) + ",";
        s.pop_back();
        BENCH_N("split(100段CSV, \",\") 长字符串", {
            volatile auto v = str::split(s, ",");
            (void)v;
        });
    }
    BENCH_SEP();

    // ---- 13. split skip_empty ----
    {
        std::string s = "a,,b,,c,,d,,e,,f,,g";
        BENCH_N("split(..., skip_empty=true)", {
            volatile auto v = str::split(s, ",", true);
            (void)v;
        });
    }
    BENCH_SEP();

    // ---- 14. rsplit ----
    // 设计意图：反向分割使用 rfind + stack，开销略大于正向 split
    {
        std::string s = "a,b,c,d,e,f,g,h,i,j";
        BENCH_N("rsplit(\"a,b,c,...\", \",\")", {
            volatile auto v = str::rsplit(s, ",");
            (void)v;
        });
    }
    BENCH_SEP();

    // ---- 15. splitAny ----
    // 设计意图：find_first_of 多字符集搜索，逐字符判断
    {
        std::string s = "a, b; c| d\t e\n f";
        BENCH_N("splitAny(\"a, b; c|...\", \",;|\\t\\n\")", {
            volatile auto v = str::splitAny(s, ",;|\t\n");
            (void)v;
        });
    }
    BENCH_SEP();

    // ======================================================================
    // trim / pad 系列 — 字符串变换操作
    // ======================================================================

    // ---- 16. trim 空白字符 ----
    // 设计意图：find_first_not_of + erase，两次线性扫描
    {
        std::string s = "   \t  hello world  \n  ";
        BENCH_N("trim() 空白字符", {
            volatile auto r = str::trim(s);
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 17. trim 自定义字符 ----
    {
        std::string s = "___hello world___";
        BENCH_N("trim(\"___\", \"_\") 自定义字符", {
            volatile auto r = str::trim(s, "_");
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 18. lpad ----
    // 设计意图：insert(0, count, char)，触发 string 头部内存移动
    {
        std::string s = "hello";
        BENCH_N("lpad(\"hello\", 80, ' ')", {
            volatile auto r = str::lpad(s, 80, ' ');
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 19. rpad ----
    // 设计意图：insert(end, count, char)，尾部追加
    {
        std::string s = "hello";
        BENCH_N("rpad(\"hello\", 80, ' ')", {
            volatile auto r = str::rpad(s, 80, ' ');
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 20. ordinalize ----
    // 设计意图：取模判断 + toStr + append，轻量级组合操作
    {
        BENCH_N("ordinalize(113)", {
            volatile auto s = str::ordinalize(113);
            (void)s;
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidostrbench

#endif // BENCH_STR_HPP

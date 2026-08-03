/*  str 模块单元测试
    分类：格式转换(ordinalize/toStr/format) | 字符串分割(split) | 修剪填充(trim/pad)  */
#pragma once
#ifndef STR_TEST_HPP_BOKUMEIDOCPP
#define STR_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/str.hpp"

using namespace meido;
namespace _meidostrcheck
{

inline void ordinalizeTest()
{
    MEIDO_ASSERT(str::ordinalize(1) == "1st" && str::ordinalize(2) == "2nd" && str::ordinalize(3) == "3rd" && str::ordinalize(10) == "10th");
    MEIDO_ASSERT(str::ordinalize(21) == "21st" && str::ordinalize(22) == "22nd" && str::ordinalize(-23) == "-23rd");
    MEIDO_ASSERT(str::ordinalize(11) == "11th" && str::ordinalize(12) == "12th" && str::ordinalize(-13) == "-13th");
}

inline void toStrTest()
{
    MEIDO_ASSERT(str::toStr(std::vector<int>({1, 2, 3})) == "{1, 2, 3}");
    MEIDO_ASSERT(str::toStr(std::vector<std::map<int, int>>({{{1, 2}, {3, 4}}})) == "{{1:2, 3:4}}");
    auto tp = std::tuple<std::map<int, int>>({{1, 2}, {3, 4}});
    MEIDO_ASSERT(str::toStr(tp) == "{{1:2, 3:4}}");

    const char* s1 = "hello";
    const char volatile* s2 = "hello";
    const char* volatile s3 = "hello";
    const wchar_t* s4 = L"hello";
    const char* const s5 = "hello";
    volatile const char s6[] = "hello";
    const char* s7 = nullptr;
    std::wstring s8 = L"hello";
    const unsigned char* s9 = (const unsigned char*)"hello";
    const char* s10[] = {s1};
    const char** s11 = &s1;

    MEIDO_ASSERT((str::toStr(s1) == "hello") && (str::toStr(s2) == "hello") && (str::toStr(s3) == "hello"));
    MEIDO_ASSERT((str::toStr(s5) == "hello") && (str::toStr(s6) == "hello") && (str::toStr(s9) == "hello"));
    MEIDO_ASSERT(str::toStr(s10) == "{hello}");

    // 以下为无法用 MEIDO_ASSERT 精确定位的类型，使用对齐格式输出供人工验证
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! toStr(const wchar_t*): Expected| 0x... (hex address)");
        _MEIDO_INFO_RAW(buf);
        snprintf(buf, sizeof(buf), "              toStr(const wchar_t*):   Actual| %s", str::toStr(s4).c_str());
        _MEIDO_INFO_RAW(buf);
    }
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! toStr(std::wstring): Expected| {...}");
        _MEIDO_INFO_RAW(buf);
        snprintf(buf, sizeof(buf), "              toStr(std::wstring):   Actual| %s", str::toStr(s8).c_str());
        _MEIDO_INFO_RAW(buf);
    }
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! toStr(const char**): Expected| 0x... (hex address)");
        _MEIDO_INFO_RAW(buf);
        snprintf(buf, sizeof(buf), "              toStr(const char**):   Actual| %s", str::toStr(s11).c_str());
        _MEIDO_INFO_RAW(buf);
    }
}

inline void formatTest()
{
    MEIDO_ASSERT(str::format("I need {} and {}  ", 1, 2) == "I need 1 and 2  ");
    MEIDO_ASSERT(str::format("I need {}, {} and {}  ", 1, 2) == "I need 1, 2 and {}  ");
    MEIDO_ASSERT(str::format("I need {}  ", 1, 2) == "I need 1  ");
    MEIDO_ASSERT(str::format("vector: {}", std::vector<std::vector<int>>({{0, 1}, {2, 3}})) == "vector: {{0, 1}, {2, 3}}");
    MEIDO_ASSERT(str::format("I need 1 and 2", 1, 2) == "I need 1 and 2");
}

inline void splitTest()
{
    // ---- split ----
    MEIDO_ASSERT(str::split("I need {} and {}  ", "{}") == std::vector<std::string>({"I need ", " and ", "  "}));
    MEIDO_ASSERT(str::split("I need {} and {}  ", "x") == std::vector<std::string>({"I need {} and {}  "}));
    MEIDO_ASSERT(str::split("I need {} and {}  ", "{}", false, 1) == std::vector<std::string>({"I need ", " and {}  "}));
    MEIDO_ASSERT(str::split("aaaaa", "aa") == std::vector<std::string>({"", "", "a"}));
    MEIDO_ASSERT(str::split("aaaaa", "aa", true) == std::vector<std::string>({"a"}));

    // ---- rsplit ----
    MEIDO_ASSERT(str::rsplit("I need {} and {}  ", "{}") == std::vector<std::string>({"I need ", " and ", "  "}));
    MEIDO_ASSERT(str::rsplit("I need {} and {}  ", "x") == std::vector<std::string>({"I need {} and {}  "}));
    MEIDO_ASSERT(str::rsplit("I need {} and {}  ", "{}", false, 1) == std::vector<std::string>({"I need {} and ", "  "}));
    MEIDO_ASSERT(str::rsplit("aaaaa", "aa") == std::vector<std::string>({"a", "", ""}));
    MEIDO_ASSERT(str::rsplit("aaaaa", "aa", true) == std::vector<std::string>({"a"}));
    MEIDO_ASSERT(str::rsplit("aaabaa", "aa") == std::vector<std::string>({"a", "b", ""}));

    // ---- splitAny / rsplitAny ----
    MEIDO_ASSERT(str::splitAny("I need {} and {}  ", "{}") == std::vector<std::string>({"I need ", "", " and ", "", "  "}));
    MEIDO_ASSERT(str::splitAny("abcde", "cd") == std::vector<std::string>({"ab", "", "e"}));
    MEIDO_ASSERT(str::rsplitAny("I need {} and {}  ", "{}") == std::vector<std::string>({"I need ", "", " and ", "", "  "}));
    MEIDO_ASSERT(str::rsplitAny("abcde", "cd") == std::vector<std::string>({"ab", "", "e"}));
    MEIDO_ASSERT(str::rsplitAny("aaabaa", "ab", true) == std::vector<std::string>({}));
}

inline void trimTest()
{
    MEIDO_ASSERT(str::trim(" \n\t 123 \v\r") == "123" && str::trim(" \n\t 123") == "123" && str::trim("123 \v\r") == "123");
    MEIDO_ASSERT(str::ltrim(" \n\t 123 \v\r") == "123 \v\r" && str::ltrim(" \n\t 123") == "123" && str::ltrim("123 \v\r") == "123 \v\r");
    MEIDO_ASSERT(str::rtrim(" \n\t 123 \v\r") == " \n\t 123" && str::rtrim(" \n\t 123") == " \n\t 123" && str::rtrim("123 \v\r") == "123");
}

inline void padTest()
{
    MEIDO_ASSERT(str::lpad("5", 5, '0') == "00005");
    MEIDO_ASSERT(str::rpad("1.5", 3, '0') == "1.5");
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check str start--------------------");

    // ---- 1. 格式转换 ----
    ordinalizeTest();
    toStrTest();
    formatTest();

    // ---- 2. 字符串分割 ----
    splitTest();

    // ---- 3. 修剪填充 ----
    trimTest();
    padTest();

    _MEIDO_INFO_RAW("---------------------check str end---------------------\n\n");
}

}    // namespace _meidostrcheck

#endif    // !STR_TEST_HPP_BOKUMEIDOCPP

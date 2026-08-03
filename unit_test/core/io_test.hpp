/*  io 模块单元测试
    分类：容器输出(io::print) | 命令行参数解析(ArgumentParser) | INI文件读写(IniFile)  */
#pragma once
#ifndef IO_TEST_HPP_BOKUMEIDOCPP
#define IO_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/io.hpp"

using namespace meido;
namespace _meidoiocheck
{

/* 占位函数：用于 io::print 函数指针输出测试 */
inline void dummyPrintFunc(int)
{
}

inline void printTest()
{
    std::unordered_multimap<int, float> m1 = {{0, 0.1f}, {0, 1.1f}};
    std::list<float> list1 = {5, 6, 7, 8, 8, 7, 6};
    std::vector<double> vec = {1.1, 2.2, 3.3};
    std::vector<std::vector<double>> vecvec({vec, vec});
    std::forward_list<int> fl({1, 2, 3});
    // initializer_list 的支撑数组生命周期只到完整表达式结束，直接绑定会悬垂（clang -Wdangling）；
    std::initializer_list<int> initl = {1, 2, 3};

    // 容器组合输出（注意 unordered_multimap 顺序不确定，实际可能不同于预期）
    printf("Author check! io::print containers: Expected| {0:0.1, 0:1.1}(顺序可变) {5, 6, 7, 8, 8, 7, 6} {{1.1, 2.2, 3.3}, {1.1, 2.2, 3.3}} {1, 2, 3} {1, 2, 3}\n");
    io::print("              io::print containers:   Actual|", m1, list1, vecvec, fl, initl);

    // 函数指针
    printf("Author check! io::print function: Expected| void (int) \n");
    io::print("              io::print function:   Actual|", dummyPrintFunc);

    // 空容器
    printf("Author check! io::print empty vector: Expected| {}\n");
    io::print("              io::print empty vector:   Actual|", std::vector<int>());

    // 单值
    printf("Author check! io::print single int: Expected| 42\n");
    io::print("              io::print single int:   Actual|", 42);
}

inline void parseArgsTest()
{
    std::vector<char*> argv_vec;
    argv_vec.resize(11);
    argv_vec[0] = (char*)"demo";
    argv_vec[1] = (char*)"-a";
    argv_vec[2] = (char*)"nothing";
    argv_vec[3] = (char*)"-cdD";
    argv_vec[4] = (char*)"4";

    argv_vec[5] = (char*)"-A";
    argv_vec[6] = (char*)"1";
    argv_vec[7] = (char*)"--BB";
    argv_vec[8] = (char*)"2";
    argv_vec[9] = (char*)"--CC";
    argv_vec[10] = (char*)"3";
    io::ArgumentParser parser;
    int ret0 = parser.parse(argv_vec.size(), argv_vec.data(),
                            {{"-a", "--aa", "bool switch1"}, {"-b", "--bb", "bool switch2"}, {"-c", "--cc", "bool switch3"}, {"-d", "--dd", "bool switch4"}},
                            {{"-A", "--AA", "value1", "111"}, {"-B", "--BB", "value02", "222"}, {"", "--CC", "value003", "333"}, {"-D", "--DD", "value4", ""}});

    MEIDO_ASSERT(ret0 == 0);

    MEIDO_ASSERT(parser.getBoolOpt("-a"));
    MEIDO_ASSERT(parser.getBoolOpt("--aa"));
    MEIDO_ASSERT(!parser.getBoolOpt("-b"));
    MEIDO_ASSERT(!parser.getBoolOpt("--bb"));
    printf("Author check! Expected| [WARN] Got an empty flag (empty flag query test)\n");
    MEIDO_ASSERT(!parser.getBoolOpt(""));

    MEIDO_ASSERT(parser.getBoolOpt("--cc"));
    MEIDO_ASSERT(parser.getBoolOpt("-d"));

    MEIDO_ASSERT(parser.getValueOpt("-A") == "1");
    MEIDO_ASSERT(parser.getValueOpt("--BB") == "2");
    MEIDO_ASSERT(parser.getValueOpt("--CC") == "3");
    MEIDO_ASSERT(parser.getValueOpt("-D") == "4");

    // logPreset/logParsed 为库函数，输出供人工验证
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! ArgumentParser preset: Expected| see below");
        _MEIDO_INFO_RAW(buf);
    }
    parser.logPreset();
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "Author check! ArgumentParser parsed: Expected| see below");
        _MEIDO_INFO_RAW(buf);
    }
    parser.logParsed();

    // 无效预设校验
    printf("Author check! Expected| [ERROR] Invalid short flag (short flag with space test)\n");
    ret0 = parser.parse(argv_vec.size(), argv_vec.data(), {}, {{"-A ", "--AA", "value1", "111"}});
    MEIDO_ASSERT(ret0 != 0);
    printf("Author check! Expected| [ERROR] Invalid short flag (short flag only dash test)\n");
    ret0 = parser.parse(argv_vec.size(), argv_vec.data(), {}, {{"- ", "--AA", "value1", "111"}});
    MEIDO_ASSERT(ret0 != 0);
    printf("Author check! Expected| [ERROR] Invalid long flag (long flag with space test)\n");
    ret0 = parser.parse(argv_vec.size(), argv_vec.data(), {}, {{"-A", "--A A", "value1", "111"}});
    MEIDO_ASSERT(ret0 != 0);
    printf("Author check! Expected| [ERROR] Invalid long flag (short flag as long flag test)\n");
    ret0 = parser.parse(argv_vec.size(), argv_vec.data(), {}, {{"-A", "-AA", "value1", "111"}});
    MEIDO_ASSERT(ret0 != 0);
}

inline void iniFileTest()
{
    const char* test_path = "./_bokumeido_test_io.ini";

    // ===== 准备临时文件（全局 key 放在首个 section 之前） =====
    {
        std::ofstream f(test_path, std::ios::trunc);
        f << "global_key = global_value\n";
        f << "\n";
        f << "[section1]\n";
        f << "key1 = value1\n";
        f << "key2 = value2\n";
        f << "\n";
        f << "[section2]\n";
        f << "key3 = value3\n";
        f.close();
    }

    // ===== 打开并读取带 section 的 key =====
    {
        io::IniFile ini;
        MEIDO_ASSERT(ini.open(test_path) == 0);

        MEIDO_ASSERT(ini.getValue("section1", "key1") == "value1");
        MEIDO_ASSERT(ini.getValue("section1", "key2") == "value2");
        MEIDO_ASSERT(ini.getValue("section2", "key3") == "value3");

        // 无 section key（通过空字符串或直接 getValue(key)）
        MEIDO_ASSERT(ini.getValue("", "global_key") == "global_value");
        MEIDO_ASSERT(ini.getValue("global_key") == "global_value");

        // 不存在的 key / section 应返回空串
        printf("Author check! Expected| [WARN] The key:nonexist / section:nonexist is not exist (nonexistent key/section test)\n");
        MEIDO_ASSERT(ini.getValue("section1", "nonexist").empty());
        MEIDO_ASSERT(ini.getValue("nonexist", "key1").empty());

        // logContents 打印已读取的内容（无断言，仅供人工验证）
        ini.logContents();

        ini.close();
    }

    // ===== 写入新值 =====
    {
        io::IniFile ini;
        MEIDO_ASSERT(ini.open(test_path) == 0);

        // 修改已有 key
        ini.setValue("section1", "key1", std::string("new_value1"));
        // 新增 key
        ini.setValue("section1", "key_new", std::string("new_value"));
        // 带 int 类型的 value
        ini.setValue("section1", "int_key", 42);
        // 新增 section
        ini.setValue("section3", "flag", std::string("enabled"));

        // 关闭时自动保存
        MEIDO_ASSERT(ini.close() == 0);
    }

    // ===== 重新打开验证写入结果 =====
    {
        io::IniFile ini;
        MEIDO_ASSERT(ini.open(test_path) == 0);

        MEIDO_ASSERT(ini.getValue("section1", "key1") == "new_value1");
        MEIDO_ASSERT(ini.getValue("section1", "key_new") == "new_value");
        MEIDO_ASSERT(ini.getValue("section1", "int_key") == "42");
        MEIDO_ASSERT(ini.getValue("section3", "flag") == "enabled");

        // 顶层 key 不受影响
        MEIDO_ASSERT(ini.getValue("global_key") == "global_value");

        ini.close();
    }

    // ===== 打开不存在的文件 =====
    {
        io::IniFile ini;
        printf("Author check! Expected| [ERROR] Failed to open ./_nonexistent_file.ini (file does not exist, expected behavior)\n");
        MEIDO_ASSERT(ini.open("./_nonexistent_file.ini") != 0);
    }

    // ===== 重复打开 =====
    {
        io::IniFile ini;
        MEIDO_ASSERT(ini.open(test_path) == 0);
        printf("Author check! Expected| [WARN] Duplicated open (second open should fail, expected behavior)\n");
        MEIDO_ASSERT(ini.open(test_path) != 0);    // 重复打开应失败
        ini.close();
    }

    // ===== 清除临时文件 =====
    ::remove(test_path);
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check io start--------------------");

    // ---- 1. 容器输出 (io::print) ----
    printTest();

    // ---- 2. 命令行参数解析 (ArgumentParser) ----
    parseArgsTest();

    // ---- 3. INI 文件读写 (IniFile) ----
    iniFileTest();

    _MEIDO_INFO_RAW("---------------------check io end---------------------\n\n");
}

}    // namespace _meidoiocheck

#endif    // !IO_TEST_HPP_BOKUMEIDOCPP

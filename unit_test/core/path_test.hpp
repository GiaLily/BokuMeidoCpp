/*  path 模块单元测试
    分类：字符串路径操作 | 文件系统路径操作  */
#pragma once
#ifndef PATH_TEST_HPP_BOKUMEIDOCPP
#define PATH_TEST_HPP_BOKUMEIDOCPP

#include <cstdio>

#include "bokumeido/core/path.hpp"

using namespace meido;
namespace _meidopathcheck
{

// ========== 字符串操作测试（无需文件系统） ==========

inline void normPathTest()
{
    MEIDO_ASSERT(path::normPath("./abc") == "abc");
    MEIDO_ASSERT(path::normPath("./abc/../") == "abc/..");
    MEIDO_ASSERT(path::normPath("C:\\") == "C:/");
    MEIDO_ASSERT(path::normPath("a//b") == "a/b");
    MEIDO_ASSERT(path::normPath("a/./b") == "a/b");
    MEIDO_ASSERT(path::normPath("/abc/") == "/abc");
    MEIDO_ASSERT(path::normPath("/") == "/");
    MEIDO_ASSERT(path::normPath("") == "");
}

inline void splitNameTest()
{
    MEIDO_ASSERT(path::splitName("./abc/file.txt", true) == "file.txt");
    MEIDO_ASSERT(path::splitName("./abc/file.txt", false) == "file");
    MEIDO_ASSERT(path::splitName("C:\\abc\\file.txt", false) == "file");
    MEIDO_ASSERT(path::splitName("readme.md", true) == "readme.md");
    MEIDO_ASSERT(path::splitName("readme.md", false) == "readme");
    MEIDO_ASSERT(path::splitName("a.b.tar.gz", false) == "a.b.tar");
    MEIDO_ASSERT(path::splitName(".gitignore", true) == ".gitignore");
    MEIDO_ASSERT(path::splitName(".gitignore", false) == ".gitignore");
}

inline void splitExtTest()
{
    MEIDO_ASSERT(path::splitExt("./abc/file.txt") == "txt");
    MEIDO_ASSERT(path::splitExt("txt") == "");
    MEIDO_ASSERT(path::splitExt("abc.aa/txt") == "");
    MEIDO_ASSERT(path::splitExt("") == "");
    MEIDO_ASSERT(path::splitExt("a.b.tar.gz") == "gz");
    MEIDO_ASSERT(path::splitExt(".gitignore") == "gitignore");
}

inline void isAbsTest()
{
    MEIDO_ASSERT(path::isAbs("/aa/bb/cc"));
    MEIDO_ASSERT(path::isAbs("C:/aa/bb"));
    MEIDO_ASSERT(!path::isAbs("aa/bb"));
    MEIDO_ASSERT(!path::isAbs("./aa"));
}

inline void joinPathTest()
{
    MEIDO_ASSERT(path::join("aa", "bb/", "cc") == "aa/bb/cc");
    MEIDO_ASSERT(path::join("", "bb") == "/bb");
    MEIDO_ASSERT(path::join("a", "b", "c", "d") == "a/b/c/d");
}

inline void parentPathTest()
{
    MEIDO_ASSERT(path::parent("./aa/bb") == "aa");
    MEIDO_ASSERT(path::parent("./aa/../bb") == "aa/..");
    MEIDO_ASSERT(path::parent("/aa") == "");
    MEIDO_ASSERT(path::parent("aa") == "aa");
}

// ========== 文件系统测试（需临时目录） ==========

static std::string g_test_root;

inline bool initTestDir()
{
    g_test_root = "./_bokumeido_test_path";
    ::remove(g_test_root.c_str());
    if (!path::requireDirs(g_test_root + "/sub1/sub1_1")) return false;
    if (!path::requireDirs(g_test_root + "/sub2")) return false;
    if (!path::makeFile(g_test_root + "/sub1/file_a.txt")) return false;
    if (!path::makeFile(g_test_root + "/sub1/file_b.log")) return false;
    if (!path::makeFile(g_test_root + "/sub1/sub1_1/deep.txt")) return false;
    if (!path::makeFile(g_test_root + "/root_file.md")) return false;
    return true;
}

inline void cleanupTestDir()
{
    ::remove((g_test_root + "/sub1/sub1_1/deep.txt").c_str());
    ::remove((g_test_root + "/sub1/file_a.txt").c_str());
    ::remove((g_test_root + "/sub1/file_b.log").c_str());
    ::remove((g_test_root + "/root_file.md").c_str());
    ::rmdir((g_test_root + "/sub1/sub1_1").c_str());
    ::rmdir((g_test_root + "/sub1").c_str());
    ::rmdir((g_test_root + "/sub2").c_str());
    ::rmdir(g_test_root.c_str());
}

inline void existsTest()
{
    MEIDO_ASSERT(path::exists(g_test_root));
    MEIDO_ASSERT(path::exists(g_test_root + "/sub1/file_a.txt"));
    MEIDO_ASSERT(!path::exists(g_test_root + "/not_exist"));
}

inline void dirFileCheckTest()
{
    MEIDO_ASSERT(path::isDir(g_test_root + "/sub1"));
    MEIDO_ASSERT(!path::isFile(g_test_root + "/sub1"));
    MEIDO_ASSERT(path::isFile(g_test_root + "/sub1/file_a.txt"));
    MEIDO_ASSERT(!path::isDir(g_test_root + "/sub1/file_a.txt"));
    MEIDO_ASSERT(!path::isDir(g_test_root + "/nonexistent_path"));  /* 不存在的路径，预期返回 false */
    MEIDO_ASSERT(!path::isFile(g_test_root + "/nonexistent_path")); /* 不存在的路径，预期返回 false */
}

inline void isFileMatchExtsTest()
{
    MEIDO_ASSERT(path::isFileMatchExts(g_test_root + "/sub1/file_a.txt", {"txt"}));
    MEIDO_ASSERT(!path::isFileMatchExts(g_test_root + "/sub1/file_a.txt", {"md"}));
    MEIDO_ASSERT(!path::isFileMatchExts(g_test_root + "/nonexistent.txt", {"txt"}));  /* 不存在的文件，预期返回 false */
    MEIDO_ASSERT(!path::isFileMatchExts(g_test_root + "/sub1", {"txt"}));
}

inline void listDirTest()
{
    {
        auto files = path::listDir(g_test_root + "/sub1", true);
        MEIDO_ASSERT(!files.empty());
        bool has_a = false, has_b = false, has_sub = false;
        for (const auto& f : files)
        {
            if (f.find("file_a.txt") != std::string::npos) has_a = true;
            if (f.find("file_b.log") != std::string::npos) has_b = true;
            if (f.find("sub1_1") != std::string::npos) has_sub = true;
        }
        MEIDO_ASSERT(has_a && has_b && has_sub);
    }
    {
        auto files = path::listDir(g_test_root + "/sub1", false);
        MEIDO_ASSERT(!files.empty());
        bool has_a = false, has_b = false, has_sub = false;
        for (const auto& f : files)
        {
            if (f == "file_a.txt") has_a = true;
            if (f == "file_b.log") has_b = true;
            if (f == "sub1_1") has_sub = true;
        }
        MEIDO_ASSERT(has_a && has_b && has_sub);
    }
    {
        auto files = path::listDir(g_test_root + "/sub1", false, {"file_a.txt"});
        bool has_a = false;
        for (const auto& f : files)
            if (f == "file_a.txt") has_a = true;
        MEIDO_ASSERT(!has_a);
    }
    {
        printf("Author check! Expected| [WARN] The input path .../nonexistent_dir is not a folder (non-existent dir test)\n");
        auto files = path::listDir(g_test_root + "/nonexistent_dir", true);
        MEIDO_ASSERT(files.empty());
    }
}

inline void createDirFileTest()
{
    std::string new_dir = g_test_root + "/_new_dir/nested";
    std::string new_file = new_dir + "/test.ini";

    bool ok = path::requireDirs(new_dir);
    MEIDO_ASSERT(ok);
    MEIDO_ASSERT(path::isDir(new_dir));

    ok = path::makeFile(new_file);
    MEIDO_ASSERT(ok);
    MEIDO_ASSERT(path::isFile(new_file));

    MEIDO_ASSERT(path::requireDirs(new_dir));
    MEIDO_ASSERT(path::makeFile(new_file));

    ::remove(new_file.c_str());
    ::rmdir(new_dir.c_str());
    ::rmdir((g_test_root + "/_new_dir").c_str());
}

inline void walkTest()
{
    auto files = path::walk(g_test_root, true);
    MEIDO_ASSERT(!files.empty());
    bool has_a = false, has_deep = false, has_root = false;
    for (const auto& f : files)
    {
        if (f.find("file_a.txt") != std::string::npos) has_a = true;
        if (f.find("deep.txt") != std::string::npos) has_deep = true;
        if (f.find("root_file.md") != std::string::npos) has_root = true;
    }
    MEIDO_ASSERT(has_a && has_deep && has_root);

    auto name_files = path::walk(g_test_root, false);
    bool has_name = false;
    for (const auto& f : name_files)
        if (f == "deep.txt") has_name = true;
    MEIDO_ASSERT(has_name);

    printf("Author check! Expected| [WARN] The input path .../nonexistent_path is not a folder (non-existent walk test)\n");
    auto nonexistent_files = path::walk(g_test_root + "/nonexistent_path", true);
    MEIDO_ASSERT(nonexistent_files.empty());
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check path start--------------------");

    // ---- 1. 字符串路径操作 ----
    normPathTest();
    splitNameTest();
    splitExtTest();
    isAbsTest();
    joinPathTest();
    parentPathTest();

    // ---- 2. 文件系统路径操作 ----
    if (initTestDir())
    {
        existsTest();
        dirFileCheckTest();
        isFileMatchExtsTest();
        listDirTest();
        createDirFileTest();
        walkTest();
        cleanupTestDir();
    }

    _MEIDO_INFO_RAW("---------------------check path end---------------------\n\n");
}

}    // namespace _meidopathcheck

#endif    // !PATH_TEST_HPP_BOKUMEIDOCPP

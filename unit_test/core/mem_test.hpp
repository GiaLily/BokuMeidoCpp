/*  mem 模块单元测试
    分类：构造 | construct/destroy | 池耗尽 | 非默认构造类型  */
#pragma once
#ifndef MEM_TEST_HPP_BOKUMEIDOCPP
#define MEM_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/mem.hpp"

using namespace meido;
namespace _meidomemcheck
{

// ========== 编译期检查 ==========

static_assert(!std::is_copy_constructible<mem::ObjectPool<int>>::value, "assert failed!");
static_assert(!std::is_copy_assignable<mem::ObjectPool<int>>::value, "assert failed!");

// ========== 辅助类型 ==========

/* 非默认可构造类型：验证 construct 转发构造参数 */
class NonDefaultConstructible
{
public:
    NonDefaultConstructible(int a, const std::string& b) :
        a_(a), b_(b) {}

    int a() const { return a_; }
    const std::string& b() const { return b_; }

private:
    int a_;
    std::string b_;
};

// ========== 测试函数 ==========

/* 默认构造和容量查询 */
inline void ConstructionTest()
{
    // 容量为 0 的池
    {
        printf("Author check! Expected| [WARN] ObjectPool capacity is 0, all construct() will return nullptr (zero capacity test)\n");
        mem::ObjectPool<int> pool(0);
        MEIDO_ASSERT(pool.capacity() == 0);
        MEIDO_ASSERT(pool.available() == 0);
    }
    // 正常容量
    {
        mem::ObjectPool<int> pool(5);
        MEIDO_ASSERT(pool.capacity() == 5);
        MEIDO_ASSERT(pool.available() == 5);
    }
}

/* 基本 construct/destroy 循环 */
inline void ConstructDestroyTest()
{
    mem::ObjectPool<int> pool(3);

    // construct 三个对象
    int* a = pool.construct();
    int* b = pool.construct();
    int* c = pool.construct();
    MEIDO_ASSERT(a != nullptr);
    MEIDO_ASSERT(b != nullptr);
    MEIDO_ASSERT(c != nullptr);
    MEIDO_ASSERT(pool.available() == 0);

    // 验证对象可正常使用
    *a = 42;
    *b = 99;
    *c = -1;
    MEIDO_ASSERT(*a == 42);
    MEIDO_ASSERT(*b == 99);
    MEIDO_ASSERT(*c == -1);

    // destroy 后 available 恢复，检查返回值
    MEIDO_ASSERT(pool.destroy(a) == 0);
    MEIDO_ASSERT(pool.available() == 1);
    MEIDO_ASSERT(pool.destroy(b) == 0);
    MEIDO_ASSERT(pool.available() == 2);
    MEIDO_ASSERT(pool.destroy(c) == 0);
    MEIDO_ASSERT(pool.available() == 3);
}

/* 池满返回 nullptr */
inline void PoolExhaustionTest()
{
    mem::ObjectPool<int> pool(2);

    int* a = pool.construct();
    int* b = pool.construct();
    int* c = pool.construct();
    MEIDO_ASSERT(a != nullptr);
    MEIDO_ASSERT(b != nullptr);
    MEIDO_ASSERT(c == nullptr);    // 池满

    // destroy 后重新可用，检查返回值
    MEIDO_ASSERT(pool.destroy(a) == 0);
    int* d = pool.construct();
    MEIDO_ASSERT(d != nullptr);
    MEIDO_ASSERT(d == a);          // 应复用同一内存地址

    MEIDO_ASSERT(pool.destroy(b) == 0);
    MEIDO_ASSERT(pool.destroy(d) == 0);
}

/* 非默认可构造类型 */
inline void NonDefaultConstructibleTest()
{
    mem::ObjectPool<NonDefaultConstructible> pool(2);

    // 传参构造
    NonDefaultConstructible* obj = pool.construct(42, std::string("hello"));
    MEIDO_ASSERT(obj != nullptr);
    MEIDO_ASSERT(obj->a() == 42);
    MEIDO_ASSERT(obj->b() == "hello");

    MEIDO_ASSERT(pool.destroy(obj) == 0);
}

/* destroy(nullptr) 返回 -1 */
inline void DestroyNullptrTest()
{
    mem::ObjectPool<int> pool(1);
    printf("Author check! Expected| [WARN] ObjectPool::destroy called with pointer:0 not belonging to this pool (nullptr destroy test)\n");
    MEIDO_ASSERT(pool.destroy(nullptr) == -1);
    MEIDO_INFO_RAW("DestroyNullptrTest passed");
}

/* destroy 不属于本池的指针返回 -1 */
inline void DestroyWrongPointerTest()
{
    mem::ObjectPool<int> pool(1);
    int fake_obj = 0;
    printf("Author check! Expected| [WARN] ObjectPool::destroy called with pointer:... not belonging to this pool (wrong pointer destroy test)\n");
    MEIDO_ASSERT(pool.destroy(&fake_obj) == -1);
    MEIDO_INFO_RAW("DestroyWrongPointerTest passed");
}

// ========== 入口 ==========

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check mem start--------------------");
    ConstructionTest();
    ConstructDestroyTest();
    PoolExhaustionTest();
    NonDefaultConstructibleTest();
    DestroyNullptrTest();
    DestroyWrongPointerTest();
    _MEIDO_INFO_RAW("\n--------------------check mem end--------------------");
}

} // namespace _meidomemcheck

#endif // !MEM_TEST_HPP_BOKUMEIDOCPP

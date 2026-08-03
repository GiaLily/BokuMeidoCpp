/*  datastruct 模块单元测试
    分类：构造 | 入队出队 | 边界条件 | 移动语义 | 特殊类型 | 环形绕回  */
#pragma once
#ifndef DATASTRUCT_TEST_HPP_BOKUMEIDOCPP
#define DATASTRUCT_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/datastruct.hpp"

using namespace meido;
namespace _meidodscheck
{

// ========== 编译期检查 ==========

// 禁止拷贝
static_assert(!std::is_copy_constructible<ds::CircularQueue<int>>::value, "assert failed!");
static_assert(!std::is_copy_assignable<ds::CircularQueue<int>>::value, "assert failed!");

// ========== 辅助类型 ==========

/* 可拷贝不可移动的辅助类型：验证 CircularQueue 对此类元素能否正常拷贝入队 */
class CopyableNonMovable
{
public:
    CopyableNonMovable() {};
    CopyableNonMovable(const CopyableNonMovable& obj) = default;
    CopyableNonMovable(CopyableNonMovable&& obj) = delete;
    CopyableNonMovable& operator=(const CopyableNonMovable& obj) = default;
    CopyableNonMovable& operator=(CopyableNonMovable&& obj) = delete;
};

/* 移动操作为 noexcept(false) 的辅助类型：验证 CircularQueue 对此类异常安全移动能否正确处理 */
class ThrowingMoveType
{
public:
    ThrowingMoveType() {};
    ThrowingMoveType(const ThrowingMoveType& obj) = delete;
    ThrowingMoveType(ThrowingMoveType&& obj) noexcept(false) {};
    ThrowingMoveType& operator=(const ThrowingMoveType& obj) = delete;
    ThrowingMoveType& operator=(ThrowingMoveType&& obj) noexcept(false)
    {
        return *this;
    };
};

// ========== 测试函数 ==========

/* 默认构造测试 */
inline void DefaultConstructionTest()
{
    ds::CircularQueue<int> q;
    MEIDO_ASSERT(q.empty());
    MEIDO_ASSERT(q.full());    // 容量为 0
}

/* 有参构造测试 */
inline void ConstructionTest()
{
    // 0 容量
    {
        ds::CircularQueue<int> q0{0};
        MEIDO_ASSERT(q0.empty());
        MEIDO_ASSERT(q0.full());
    }
    // 正常容量
    {
        ds::CircularQueue<int> q{3};
        MEIDO_ASSERT(q.empty());
        MEIDO_ASSERT(!q.full());
    }
    // 容量为 1
    {
        ds::CircularQueue<int> q{1};
        MEIDO_ASSERT(q.empty());
        MEIDO_ASSERT(!q.full());
    }
}

/* enqueue(const T&) + tryDequeue 测试 */
inline void LvalueEnqueueDequeueTest()
{
    ds::CircularQueue<int> q{3};
    int a = 1, b = 2, c = 3;
    q.enqueue(a);
    q.enqueue(b);
    q.enqueue(c);
    MEIDO_ASSERT(q.full());
    MEIDO_ASSERT(!q.empty());

    int d = 4;
    q.enqueue(d);
    MEIDO_ASSERT(q.full());

    int dst = 0;
    MEIDO_ASSERT(q.tryDequeue(dst));
    MEIDO_ASSERT(dst == 2);
    MEIDO_ASSERT(q.tryDequeue(dst));
    MEIDO_ASSERT(dst == 3);
    MEIDO_ASSERT(q.tryDequeue(dst));
    MEIDO_ASSERT(dst == 4);
    MEIDO_ASSERT(!q.tryDequeue(dst));
    MEIDO_ASSERT(q.empty());
}

/* enqueue(T&&) + tryDequeue 测试 */
inline void RvalueEnqueueDequeueTest()
{
    // 基本类型右值
    {
        ds::CircularQueue<int> q{2};
        q.enqueue(10);
        q.enqueue(20);
        MEIDO_ASSERT(q.full());
        int dst = 0;
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(dst == 10);
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(dst == 20);
    }
    // 仅移动类型（unique_ptr）
    {
        ds::CircularQueue<std::unique_ptr<int>> q{3};
        q.enqueue(std::unique_ptr<int>(new int(100)));
        q.enqueue(std::unique_ptr<int>(new int(200)));
        q.enqueue(std::unique_ptr<int>(new int(300)));
        MEIDO_ASSERT(q.full());

        std::unique_ptr<int> dst;
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(dst != nullptr);
        MEIDO_ASSERT(*dst == 100);
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(*dst == 200);
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(*dst == 300);
        MEIDO_ASSERT(!q.tryDequeue(dst));
    }
}

/* 空队列出队测试 */
inline void DequeueFromEmptyTest()
{
    ds::CircularQueue<int> q{3};
    int dst = 0;
    MEIDO_ASSERT(!q.tryDequeue(dst));
    MEIDO_ASSERT(dst == 0);
}

/* 容量为 1 的边界测试 */
inline void CapacityOneTest()
{
    ds::CircularQueue<int> q{1};
    MEIDO_ASSERT(q.empty());
    MEIDO_ASSERT(!q.full());

    q.enqueue(10);
    MEIDO_ASSERT(!q.empty());
    MEIDO_ASSERT(q.full());

    q.enqueue(20);
    MEIDO_ASSERT(q.full());

    int dst = 0;
    MEIDO_ASSERT(q.tryDequeue(dst));
    MEIDO_ASSERT(dst == 20);
    MEIDO_ASSERT(q.empty());
    MEIDO_ASSERT(!q.full());
}

/* 0 容量边界测试 */
inline void ZeroCapacityTest()
{
    ds::CircularQueue<int> q0{0};
    MEIDO_ASSERT(q0.empty());
    MEIDO_ASSERT(q0.full());
    q0.enqueue(1);
    q0.enqueue(2);
    int dst = -1;
    MEIDO_ASSERT(!q0.tryDequeue(dst));
    MEIDO_ASSERT(dst == -1);
}

/* empty / full 状态跟踪测试 */
inline void StateTrackingTest()
{
    ds::CircularQueue<int> q{2};
    MEIDO_ASSERT(q.empty());
    MEIDO_ASSERT(!q.full());

    q.enqueue(1);
    MEIDO_ASSERT(!q.empty());
    MEIDO_ASSERT(!q.full());

    q.enqueue(2);
    MEIDO_ASSERT(!q.empty());
    MEIDO_ASSERT(q.full());

    int dst = 0;
    q.tryDequeue(dst);
    MEIDO_ASSERT(!q.empty());
    MEIDO_ASSERT(!q.full());

    q.tryDequeue(dst);
    MEIDO_ASSERT(q.empty());
    MEIDO_ASSERT(!q.full());
}

/* 移动语义测试 */
inline void MoveSemanticsTest()
{
    // 非空队列 → 移动构造
    {
        ds::CircularQueue<int> q{3};
        q.enqueue(1);
        q.enqueue(2);
        q.enqueue(3);

        ds::CircularQueue<int> q2 = std::move(q);
        MEIDO_ASSERT(q2.full());
        int dst = 0;
        MEIDO_ASSERT(q2.tryDequeue(dst));
        MEIDO_ASSERT(dst == 1);
        MEIDO_ASSERT(q.empty());
    }
    // 非空队列 → 移动赋值
    {
        ds::CircularQueue<int> q{3};
        q.enqueue(10);
        q.enqueue(20);

        ds::CircularQueue<int> q2{5};
        q2 = std::move(q);
        MEIDO_ASSERT(!q2.empty());
        int dst = 0;
        MEIDO_ASSERT(q2.tryDequeue(dst));
        MEIDO_ASSERT(dst == 10);
        MEIDO_ASSERT(q2.tryDequeue(dst));
        MEIDO_ASSERT(dst == 20);
        MEIDO_ASSERT(!q2.tryDequeue(dst));
        MEIDO_ASSERT(q.empty());
    }
    // 空队列 → 移动构造
    {
        ds::CircularQueue<int> q;
        ds::CircularQueue<int> q2 = std::move(q);
        MEIDO_ASSERT(q2.empty());
        MEIDO_ASSERT(q2.full());
    }
    // 空队列 → 移动赋值
    {
        ds::CircularQueue<int> q;
        ds::CircularQueue<int> q2{3};
        q2 = std::move(q);
        MEIDO_ASSERT(q2.empty());
        MEIDO_ASSERT(q2.full());
    }
    // 自移动赋值
    {
        ds::CircularQueue<int> q{2};
        q.enqueue(1);
        q.enqueue(2);
        ds::CircularQueue<int>& ref = q;
        q = std::move(ref);
        MEIDO_ASSERT(!q.empty());
        int dst = 0;
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(dst == 1);
    }
}

/* 特殊类型测试 */
inline void SpecialTypeTest()
{
    // CopyableNonMovable：可拷贝不可移动——验证 tryDequeue 走 copy 分派路径
    {
        CopyableNonMovable src;
        ds::CircularQueue<CopyableNonMovable> q{3};
        q.enqueue(src);
        q.enqueue(std::move(src));

        CopyableNonMovable dst;
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(q.tryDequeue(dst));
        MEIDO_ASSERT(!q.tryDequeue(dst));
        MEIDO_ASSERT(q.empty());
    }
    // ThrowingMoveType：noexcept(false) 移动——验证异常安全移动路径
    {
        ds::CircularQueue<ThrowingMoveType> q{3};
        q.enqueue(ThrowingMoveType{});
        q.enqueue(ThrowingMoveType{});
        q.enqueue(ThrowingMoveType{});
        MEIDO_ASSERT(q.full());

        ThrowingMoveType dst;
        MEIDO_ASSERT(q.tryDequeue(dst));
    }
}

/* 环形缓冲区 wrap-around 行为测试 */
inline void WrapAroundTest()
{
    ds::CircularQueue<int> q{3};
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    int dst = 0;
    q.tryDequeue(dst);
    MEIDO_ASSERT(dst == 1);
    q.tryDequeue(dst);
    MEIDO_ASSERT(dst == 2);
    q.enqueue(4);
    q.enqueue(5);
    MEIDO_ASSERT(q.full());
    q.tryDequeue(dst);
    MEIDO_ASSERT(dst == 3);
    q.tryDequeue(dst);
    MEIDO_ASSERT(dst == 4);
    q.tryDequeue(dst);
    MEIDO_ASSERT(dst == 5);
    MEIDO_ASSERT(q.empty());
}

/* 批量运行 */
inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check ds start--------------------");

    // ---- 1. 构造 ----
    DefaultConstructionTest();
    ConstructionTest();

    // ---- 2. 入队出队 ----
    LvalueEnqueueDequeueTest();
    RvalueEnqueueDequeueTest();
    DequeueFromEmptyTest();

    // ---- 3. 边界条件 ----
    CapacityOneTest();
    ZeroCapacityTest();
    StateTrackingTest();

    // ---- 4. 移动语义 ----
    MoveSemanticsTest();
    SpecialTypeTest();

    // ---- 5. 环形绕回 ----
    WrapAroundTest();

    _MEIDO_INFO_RAW("---------------------check ds end---------------------\n\n");
}

}    // namespace _meidodscheck

#endif    // !DATASTRUCT_TEST_HPP_BOKUMEIDOCPP

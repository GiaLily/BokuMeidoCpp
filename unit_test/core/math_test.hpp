/*  math 模块单元测试
    分类：数值函数 / 数学常量 / Rect几何运算   */
#pragma once
#ifndef MATH_TEST_HPP_BOKUMEIDOCPP
#define MATH_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/math.hpp"

using namespace meido;
namespace _meidomathcheck
{

// ========== Rect 测试 ==========

/* Rect 构造测试：两点构造与坐标自动调整 */
inline void RectConstructionTest()
{
    // 两点构造——坐标自动调整（构造参数顺序: x1, y1, x2, y2）
    {
        math::Rect<int> r(5, 1, 3, 4);   // x1=5, y1=1, x2=3, y2=4 → xMin=3, xMax=5, yMin=1, yMax=4
        MEIDO_ASSERT(r.xMin() == 3);
        MEIDO_ASSERT(r.yMin() == 1);
        MEIDO_ASSERT(r.xMax() == 5);
        MEIDO_ASSERT(r.yMax() == 4);
    }
    // 点矩形（起点终点相同）
    {
        math::Rect<int> r(2, 3, 2, 3);
        MEIDO_ASSERT(r.xMin() == 2 && r.xMax() == 2);
        MEIDO_ASSERT(r.yMin() == 3 && r.yMax() == 3);
    }
    // float 类型
    {
        math::Rect<float> r(1.5f, 0.5f, 3.5f, 2.5f);
        MEIDO_ASSERT(r.xMin() == 1.5f);
        MEIDO_ASSERT(r.xMax() == 3.5f);
        MEIDO_ASSERT(r.yMin() == 0.5f);
        MEIDO_ASSERT(r.yMax() == 2.5f);
    }
    // double 类型
    {
        math::Rect<double> r(0.0, 0.0, 10.0, 10.0);
        MEIDO_ASSERT(r.xMin() == 0.0);
        MEIDO_ASSERT(r.xMax() == 10.0);
        MEIDO_ASSERT(r.yMin() == 0.0);
        MEIDO_ASSERT(r.yMax() == 10.0);
    }
}

/* Rect 相等/不等比较测试 */
inline void RectComparisonTest()
{
    math::Rect<int> r1(1, 2, 4, 6);    // [1,4]x[2,6]
    math::Rect<int> r2(1, 2, 4, 6);
    math::Rect<int> r3(1, 2, 4, 7);
    MEIDO_ASSERT(r1 == r2);
    MEIDO_ASSERT(r1 != r3);
    MEIDO_ASSERT(!(r1 == r3));
}

/* Rect 拷贝构造与赋值测试 */
inline void RectCopyTest()
{
    math::Rect<int> r1(1, 2, 4, 6);
    math::Rect<int> r2(r1);
    MEIDO_ASSERT(r2 == r1);

    math::Rect<int> r3(0, 0, 0, 0);
    r3 = r1;
    MEIDO_ASSERT(r3 == r1);
}

/* Rect clipTo 测试 */
inline void RectClipToTest()
{
    math::Rect<int> r_a(0, 0, 10, 10);   // 大矩形 [0,10]x[0,10]
    math::Rect<int> r_b(3, 2, 8, 7);     // 内部小矩形

    // 有交集
    {
        math::Rect<int> clipped(0, 0, 0, 0);
        MEIDO_ASSERT(r_a.clipTo(r_b, clipped));
        MEIDO_ASSERT(clipped.xMin() == 3);
        MEIDO_ASSERT(clipped.xMax() == 8);
        MEIDO_ASSERT(clipped.yMin() == 2);
        MEIDO_ASSERT(clipped.yMax() == 7);
    }
    // 完全包含（反向 clip）
    {
        math::Rect<int> clipped(0, 0, 0, 0);
        MEIDO_ASSERT(r_b.clipTo(r_a, clipped));
        MEIDO_ASSERT(clipped.xMin() == 3);
        MEIDO_ASSERT(clipped.xMax() == 8);
    }
    // 无交集
    {
        math::Rect<int> r_c(20, 20, 30, 30);
        math::Rect<int> clipped(0, 0, 0, 0);
        MEIDO_ASSERT(!r_a.clipTo(r_c, clipped));
    }
    // 线交集（一条边重叠）
    {
        math::Rect<int> r_d(0, 10, 10, 20);
        math::Rect<int> clipped(0, 0, 0, 0);
        MEIDO_ASSERT(r_a.clipTo(r_d, clipped));
        MEIDO_ASSERT(clipped.xMin() == 0);
        MEIDO_ASSERT(clipped.xMax() == 10);
        MEIDO_ASSERT(clipped.yMin() == 10);
        MEIDO_ASSERT(clipped.yMax() == 10);
    }
    // 点交集（一个角重叠）
    {
        math::Rect<int> r_e(10, 10, 20, 20);
        math::Rect<int> clipped(0, 0, 0, 0);
        MEIDO_ASSERT(r_a.clipTo(r_e, clipped));
        MEIDO_ASSERT(clipped.xMin() == 10);
        MEIDO_ASSERT(clipped.xMax() == 10);
        MEIDO_ASSERT(clipped.yMin() == 10);
        MEIDO_ASSERT(clipped.yMax() == 10);
    }
}

/* Rect contains 测试 */
inline void RectContainsTest()
{
    math::Rect<int> r_a(0, 0, 10, 10);   // 大矩形 [0,10]x[0,10]

    // contains(x, y) —— 内部点包含
    MEIDO_ASSERT(r_a.contains(5, 5));
    // contains(x, y) —— 边界点包含
    MEIDO_ASSERT(r_a.contains(0, 0));
    MEIDO_ASSERT(r_a.contains(10, 10));
    // contains(x, y) —— 外部点不包含
    MEIDO_ASSERT(!r_a.contains(-1, 5));
    MEIDO_ASSERT(!r_a.contains(5, 11));
    // contains(Rect) —— 完全包含
    {
        math::Rect<int> r_inner(2, 2, 8, 8);
        MEIDO_ASSERT(r_a.contains(r_inner));
    }
    // contains(Rect) —— 边界重合也视为包含
    {
        math::Rect<int> r_same(0, 0, 10, 10);
        MEIDO_ASSERT(r_a.contains(r_same));
    }
    // contains(Rect) —— 部分超出则不包含
    {
        math::Rect<int> r_out(5, 5, 15, 15);
        MEIDO_ASSERT(!r_a.contains(r_out));
    }



}

/* 数值函数测试：safeAbs（安全绝对值，对 INT_MIN/LLONG_MIN 等最值无溢出风险） */
// SFINAE trait：检测 safeAbs 是否对类型 T 可用（bool/浮点等应被拒绝）
template <class T, class = void>
struct IsSafeAbsCallable : std::false_type
{
};
template <class T>
struct IsSafeAbsCallable<T, decltype(math::safeAbs(std::declval<T>()), void())> : std::true_type
{
};

inline void SafeAbsTest()
{
    // constexpr 编译期求值
    static_assert(math::safeAbs(-5) == 5u, "safeAbs must be constexpr-evaluable");
    static_assert(math::safeAbs(0) == 0u, "safeAbs must be constexpr-evaluable");
    static_assert(math::safeAbs(-2147483647 - 1) == 2147483648u, "safeAbs(INT_MIN) must not overflow");
    static_assert(math::safeAbs(-9223372036854775807LL - 1) == 9223372036854775808ULL,
                  "safeAbs(LLONG_MIN) must not overflow");

    // 返回类型：make_unsigned<T>
    static_assert(std::is_same<decltype(math::safeAbs(-5)), unsigned int>::value,
                  "safeAbs(int) must return unsigned int");
    static_assert(std::is_same<decltype(math::safeAbs(-5LL)), unsigned long long>::value,
                  "safeAbs(long long) must return unsigned long long");
    static_assert(std::is_same<decltype(math::safeAbs(5u)), unsigned int>::value,
                  "safeAbs(unsigned) must return unsigned int");

    // SFINAE 约束：bool / 浮点类型不可调用，int / const int 可调用
    static_assert(!IsSafeAbsCallable<bool>::value, "bool must be rejected");
    static_assert(!IsSafeAbsCallable<float>::value, "float must be rejected");
    static_assert(!IsSafeAbsCallable<double>::value, "double must be rejected");
    static_assert(IsSafeAbsCallable<int>::value, "int must be callable");
    static_assert(IsSafeAbsCallable<const int>::value, "const int must be callable");

    // int —— 正数 / 负数 / 零
    MEIDO_ASSERT(math::safeAbs(5) == 5u);
    MEIDO_ASSERT(math::safeAbs(-5) == 5u);
    MEIDO_ASSERT(math::safeAbs(0) == 0u);

    // int —— 边界最值（普通 abs 对 INT_MIN 会溢出的场景）
    MEIDO_ASSERT(math::safeAbs(std::numeric_limits<int>::min()) == 2147483648u);
    MEIDO_ASSERT(math::safeAbs(std::numeric_limits<int>::max()) == 2147483647u);

    // long long —— 边界最值
    MEIDO_ASSERT(math::safeAbs(std::numeric_limits<long long>::min()) == 9223372036854775808ULL);
    MEIDO_ASSERT(math::safeAbs(std::numeric_limits<long long>::max()) == 9223372036854775807ULL);

    // unsigned —— 无符号透传（x >= 0 分支）
    MEIDO_ASSERT(math::safeAbs(5u) == 5u);
    MEIDO_ASSERT(math::safeAbs(std::numeric_limits<unsigned int>::max()) == std::numeric_limits<unsigned int>::max());

    // 窄整型
    MEIDO_ASSERT(math::safeAbs(static_cast<short>(-8)) == 8u);
    MEIDO_ASSERT(math::safeAbs(static_cast<signed char>(-3)) == 3u);
    MEIDO_ASSERT(math::safeAbs(-6L) == 6u);

    // cv 限定输入（SFINAE 经 remove_cv 判定，支持 cv 限定类型）
    const int ci = -7;
    MEIDO_ASSERT(math::safeAbs(ci) == 7u);              // 按值传递，T 推导为 int
    MEIDO_ASSERT(math::safeAbs<const int>(-7) == 7u);   // 显式指定 T = const int
}

/* 数值函数测试：clamp */
inline void ClampTest()
{
    // int —— 正常范围
    MEIDO_ASSERT(math::clamp(5, 0, 10) == 5);
    // int —— 小于下限
    MEIDO_ASSERT(math::clamp(-5, 0, 10) == 0);
    // int —— 大于上限
    MEIDO_ASSERT(math::clamp(15, 0, 10) == 10);
    // int —— 边界值
    MEIDO_ASSERT(math::clamp(0, 0, 10) == 0);
    MEIDO_ASSERT(math::clamp(10, 0, 10) == 10);

    // unsigned
    MEIDO_ASSERT(math::clamp(5u, 0u, 10u) == 5u);
    MEIDO_ASSERT(math::clamp(15u, 0u, 10u) == 10u);

    // float
    MEIDO_ASSERT(math::clamp(3.14f, 0.0f, 1.0f) == 1.0f);
    MEIDO_ASSERT(math::clamp(-1.0f, 0.0f, 1.0f) == 0.0f);

    // double
    MEIDO_ASSERT(math::clamp(0.5, 0.0, 1.0) == 0.5);
}

/* 数值函数测试：sign */
inline void SignTest()
{
    // int —— 正
    MEIDO_ASSERT(math::sign(5) == 1);
    // int —— 负
    MEIDO_ASSERT(math::sign(-3) == -1);
    // int —— 零
    MEIDO_ASSERT(math::sign(0) == 0);

    // unsigned —— 始终非负
    MEIDO_ASSERT(math::sign(5u) == 1);
    MEIDO_ASSERT(math::sign(0u) == 0);

    // float
    MEIDO_ASSERT(math::sign(3.14f) == 1);
    MEIDO_ASSERT(math::sign(-2.5f) == -1);
    MEIDO_ASSERT(math::sign(0.0f) == 0);
    // 负零
    MEIDO_ASSERT(math::sign(-0.0f) == 0);

    // double
    MEIDO_ASSERT(math::sign(1.0) == 1);
    MEIDO_ASSERT(math::sign(-1.0) == -1);
}

/* 数值函数测试：isPowerOfTwo */
inline void IsPowerOfTwoTest()
{
    // 2的幂
    MEIDO_ASSERT(math::isPowerOfTwo(1));
    MEIDO_ASSERT(math::isPowerOfTwo(2));
    MEIDO_ASSERT(math::isPowerOfTwo(4));
    MEIDO_ASSERT(math::isPowerOfTwo(8));
    MEIDO_ASSERT(math::isPowerOfTwo(1024));

    // 非2的幂
    MEIDO_ASSERT(!math::isPowerOfTwo(0));
    MEIDO_ASSERT(!math::isPowerOfTwo(3));
    MEIDO_ASSERT(!math::isPowerOfTwo(5));
    MEIDO_ASSERT(!math::isPowerOfTwo(1023));

    // 无符号
    MEIDO_ASSERT(math::isPowerOfTwo(1u));
    MEIDO_ASSERT(!math::isPowerOfTwo(0u));
}

/* 数学常量测试 */
inline void MathConstTest()
{
    // float 精度
    MEIDO_ASSERT(math::pi<float>() > 3.14f);
    MEIDO_ASSERT(math::pi<float>() < 3.15f);
    // double 精度
    MEIDO_ASSERT(math::pi<double>() > 3.1415926);
    MEIDO_ASSERT(math::pi<double>() < 3.1415927);

    // e
    MEIDO_ASSERT(math::e<double>() > 2.71828);
    MEIDO_ASSERT(math::e<double>() < 2.71829);

    // phi
    MEIDO_ASSERT(math::phi<double>() > 1.61803);
    MEIDO_ASSERT(math::phi<double>() < 1.61804);

    // 整数截断
    MEIDO_ASSERT(math::pi<int>() == 3);
    MEIDO_ASSERT(math::e<int>() == 2);
    MEIDO_ASSERT(math::phi<int>() == 1);
}

/* Rect 输出格式测试 */
inline void RectOutputTest()
{
    // 有效矩形
    {
        math::Rect<int> r(1, 2, 4, 6);
        std::ostringstream oss;
        oss << r;
        MEIDO_ASSERT(oss.str() == "[1 2 4 6]");
    }
    // float 类型
    {
        math::Rect<float> r(1.5f, 0.5f, 3.5f, 2.5f);
        std::ostringstream oss;
        oss << r;
        MEIDO_ASSERT(oss.str() == "[1.5 0.5 3.5 2.5]");
    }
}

/* 批量运行 */
inline void check()
{
    MEIDO_INFO_RAW("\n--------------------check math start--------------------");

    // ---- 1. 数值函数测试 ----
    SafeAbsTest();
    ClampTest();
    SignTest();
    IsPowerOfTwoTest();
    MathConstTest();

    // ---- 2. Rect 测试 ----
    RectConstructionTest();
    RectComparisonTest();
    RectCopyTest();
    RectClipToTest();
    RectContainsTest();
    RectOutputTest();


    MEIDO_INFO_RAW("---------------------check math end---------------------\n\n");
}

}    // namespace _meidomathcheck

#endif    // !MATH_TEST_HPP_BOKUMEIDOCPP

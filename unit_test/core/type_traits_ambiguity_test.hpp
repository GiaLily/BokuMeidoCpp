/*============================================================================*/
/*                                                                            */
/*   ⚠️  标准库 type_traits 歧义性验证测试  ⚠️                                */
/*                                                                            */
/*   验证和记录 std::type_traits 中容易产生歧义的检查结果。                    */
/*   重点关注: 当 T(T&&) = delete 或未声明时, is_move_xxx 的真实行为,         */
/*   以及 is_copy_xxx 实际检测的是 const T& 而非 T& 这一事实。                */
/*                                                                            */
/*   每类测试格式:                                                             */
/*     普通行为: [描述] → 预期值                                               */
/*     ⚠️ 反直觉: [描述] → 预期值 —— !!! 为什么反直觉 !!!                    */
/*                                                                            */
/*   快速索引:                                                                 */
/*     [构造] §1.1 ~ §1.9 — 构造测试 9 种组合                                 */
/*     [赋值] §2.1 ~ §2.5 — 赋值测试 5 种组合                                 */
/*     [综合] §3.1 ~ §3.3 — 构造+赋值综合类型                                 */
/*     [附录] §4      — 歧义性根源分析总结                                    */
/*                                                                            */
/*============================================================================*/
#pragma once
#ifndef TYPE_TRAITS_AMBIGUITY_TEST_HPP_BOKUMEIDOCPP
#define TYPE_TRAITS_AMBIGUITY_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/type.hpp"

using namespace meido;

namespace _meidotypeambiguitycheck
{

// ============================================================================
//  前瞻知识: 要检测哪些 trait 及其对应关系
// ============================================================================
//  缩写    | 全称                                          | 实际检查内容
//  --------|-----------------------------------------------|-----------------------
//  copy    | std::is_copy_constructible<T>                 | is_constructible<T, const T&>
//  move    | std::is_move_constructible<T>                 | is_constructible<T, T&&>
//  T&      | std::is_constructible<T, T&>                  | 来自 非const左值的构造能力
//  T&&     | std::is_constructible<T, T&&>                 | 来自 右值的构造能力
//  constT& | std::is_constructible<T, const T&>            | 来自 const左值的构造能力
//  Lval    | type::EachLvalueConstructibleChecker<T>        | 同 is_constructible<T, T&>
//  Rval    | type::EachRvalueConstructibleChecker<T>        | 同 is_constructible<T, T&&>
//
//  核心歧义:
//    [陷阱①] 当 T(const T&) 存在且 T(T&&) 未声明:
//             T&& → const T& 绑定, 所以 move = true
//             即使 没有移动构造函数, move 也是 true !
//    [陷阱②] 当 T(T&&) = delete 时:
//             T&& 完美匹配 delete 的 T(T&&), 不会 fallback 到 const T&
//             所以 move = false (与陷阱①不同!)
//    [陷阱③] is_copy_xxx 检查 const T&, 不是 T&
//             只有 T(T&) 时, copy = false (即使可以从非const左值构造)

// ============================================================================
//  第一组: 构造测试 (Constructor)  §1.1 ~ §1.9
//  覆盖拷贝构造/移动构造的所有有意义组合
// ============================================================================

// ---------------------------------------------------------------------------
//  §1.1  Ctor_Auto          — 完全交给编译器自动生成
//  行为: 编译器自动生成拷贝构造和移动构造
//  结果: 所有 trait 均为 true (基线对照)
// ---------------------------------------------------------------------------
struct Ctor_Auto
{
};

// ---------------------------------------------------------------------------
//  §1.2  Ctor_CopyOnly      — 只有 const T& 拷贝构造, 不声明移动构造
//  行为: 用户声明了拷贝构造 → 编译器不生成移动构造
//  ⚠️ 反直觉: 即使没有移动构造函数, move = true
//     原因: T&& 表达式绑定到 const T& 参数上, 通过拷贝构造"伪移动"
// ---------------------------------------------------------------------------
struct Ctor_CopyOnly
{
    Ctor_CopyOnly(const Ctor_CopyOnly&) {}
};

// ---------------------------------------------------------------------------
//  §1.3  Ctor_Copy_DeleteMove — const T& 拷贝构造 + T&& = delete
//  行为: 显式 delete 移动构造
//  结果: move = false
//  ⚠️ 与 §1.2 对比: 这里 move = false !
//     原因: T&& 优先匹配完美匹配的 T(T&&) (即使 delete),
//           重载决议不会 fallback 到 const T& 版本
// ---------------------------------------------------------------------------
struct Ctor_Copy_DeleteMove
{
    Ctor_Copy_DeleteMove(const Ctor_Copy_DeleteMove&) {}
    Ctor_Copy_DeleteMove(Ctor_Copy_DeleteMove&&) = delete;
};

// ---------------------------------------------------------------------------
//  §1.4  Ctor_Move_DeleteCopy — T&& 移动构造 + const T& = delete
//  行为: 显式 delete 拷贝构造, 定义移动构造
//  结果: copy = false, move = true
//  注意: 编译器看到 delete 的拷贝构造后, 不会再自动生成拷贝构造
// ---------------------------------------------------------------------------
struct Ctor_Move_DeleteCopy
{
    Ctor_Move_DeleteCopy(Ctor_Move_DeleteCopy&&) {}
    Ctor_Move_DeleteCopy(const Ctor_Move_DeleteCopy&) = delete;
};

// ---------------------------------------------------------------------------
//  §1.5  Ctor_None           — 两者全 delete
//  行为: 拷贝构造和移动构造均被 delete
//  结果: 全部为 false
// ---------------------------------------------------------------------------
struct Ctor_None
{
    Ctor_None(const Ctor_None&) = delete;
    Ctor_None(Ctor_None&&) = delete;
};

// ---------------------------------------------------------------------------
//  §1.6  Ctor_Both           — 两者都定义 (正常情况)
//  行为: 有显式的拷贝构造和移动构造
//  结果: 全部为 true (基线对照)
// ---------------------------------------------------------------------------
struct Ctor_Both
{
    Ctor_Both(const Ctor_Both&) {}
    Ctor_Both(Ctor_Both&&) {}
};

// ---------------------------------------------------------------------------
//  §1.7  Ctor_MoveOnly       — 只有 T&& 移动构造, 不声明拷贝构造
//  行为: 用户声明了移动构造 → 编译器抑制隐式拷贝构造生成
//  结果: copy = false, move = true
// ---------------------------------------------------------------------------
struct Ctor_MoveOnly
{
    Ctor_MoveOnly(Ctor_MoveOnly&&) {}
};

// ---------------------------------------------------------------------------
//  §1.8  Ctor_NonConstCopy   — 只有非 const 左值拷贝构造 T(T&)
//  行为: 只有 T(T&) 拷贝构造, 没有 T(const T&), 也没有 T(T&&)
//  ⚠️ 反直觉: is_copy_constructible = false !
//     原因: is_copy_constructible 实际检查 is_constructible<T, const T&>,
//           而类型只有 T(T&), const T& 无法绑定到 T& (除非去掉 const)
//  结论: 用 is_constructible<T, T&> 才能检测"能从非 const 左值构造"
// ---------------------------------------------------------------------------
struct Ctor_NonConstCopy
{
    Ctor_NonConstCopy(Ctor_NonConstCopy&) {}
};

// ---------------------------------------------------------------------------
//  §1.9  Ctor_Default        — 仅默认构造, 不声明拷贝/移动
//  行为: 只声明了默认构造, 编译器自动生成拷贝构造和移动构造
//  结果: 全部为 true (基线对照)
// ---------------------------------------------------------------------------
struct Ctor_Default
{
    Ctor_Default() = default;
};


// ============================================================================
//  第二组: 赋值测试 (Assignment)  §2.1 ~ §2.5
//  覆盖赋值操作符的所有有意义组合
//  原理与构造测试完全一致, 只是检查的目标从构造变为赋值
// ============================================================================

// ---------------------------------------------------------------------------
//  §2.1  Assign_CopyOnly     — 只有 const T& 赋值, 不声明移动赋值
//  ⚠️ 反直觉: 无移动赋值声明 → move = true (T&& → const T& 绑定)
// ---------------------------------------------------------------------------
struct Assign_CopyOnly
{
    Assign_CopyOnly& operator=(const Assign_CopyOnly&) { return *this; }
};

// ---------------------------------------------------------------------------
//  §2.2  Assign_Copy_DeleteMove — const T& 赋值 + T&& = delete
//  结果: move = false (完美匹配 delete 版本)
//  ⚠️ 与 §2.1 对比: 即使都有 const T& 赋值,
//     §2.1 move=T, §2.2 move=F (区别在于有无声明 T&& operator=)
// ---------------------------------------------------------------------------
struct Assign_Copy_DeleteMove
{
    Assign_Copy_DeleteMove& operator=(const Assign_Copy_DeleteMove&) { return *this; }
    Assign_Copy_DeleteMove& operator=(Assign_Copy_DeleteMove&&) = delete;
};

// ---------------------------------------------------------------------------
//  §2.3  Assign_Move_DeleteCopy — T&& 赋值 + const T& = delete
//  结果: copy = false, move = true
// ---------------------------------------------------------------------------
struct Assign_Move_DeleteCopy
{
    Assign_Move_DeleteCopy& operator=(Assign_Move_DeleteCopy&&) { return *this; }
    Assign_Move_DeleteCopy& operator=(const Assign_Move_DeleteCopy&) = delete;
};

// ---------------------------------------------------------------------------
//  §2.4  Assign_None         — 两者全 delete
//  结果: 全部为 false
// ---------------------------------------------------------------------------
struct Assign_None
{
    Assign_None& operator=(const Assign_None&) = delete;
    Assign_None& operator=(Assign_None&&) = delete;
};

// ---------------------------------------------------------------------------
//  §2.5  Assign_NonConstCopy — 只有非 const 左值赋值 T&
//  ⚠️ 反直觉: is_copy_assignable = false (它检查 const T&, 不是 T&)
//     但 is_assignable<T&, T&> = true
// ---------------------------------------------------------------------------
struct Assign_NonConstCopy
{
    Assign_NonConstCopy& operator=(Assign_NonConstCopy&) { return *this; }
};


// ============================================================================
//  第三组: 综合类型测试 §3.1 ~ §3.3
//  构造和赋值同时受影响的类型
// ============================================================================

// ---------------------------------------------------------------------------
//  §3.1  Full_CopyOnly_DeleteMove — 拷贝构造+拷贝赋值, 且 delete 移动版本
//  注释: §1.3 + §2.2 的综合体
// ---------------------------------------------------------------------------
struct Full_CopyOnly_DeleteMove
{
    Full_CopyOnly_DeleteMove(const Full_CopyOnly_DeleteMove&) {}
    Full_CopyOnly_DeleteMove(Full_CopyOnly_DeleteMove&&) = delete;
    Full_CopyOnly_DeleteMove& operator=(const Full_CopyOnly_DeleteMove&) { return *this; }
    Full_CopyOnly_DeleteMove& operator=(Full_CopyOnly_DeleteMove&&) = delete;
};

// ---------------------------------------------------------------------------
//  §3.2  Full_Both           — 拷贝和移动都定义 (正常情况)
// ---------------------------------------------------------------------------
struct Full_Both
{
    Full_Both(const Full_Both&) {}
    Full_Both(Full_Both&&) {}
    Full_Both& operator=(const Full_Both&) { return *this; }
    Full_Both& operator=(Full_Both&&) { return *this; }
};

// ---------------------------------------------------------------------------
//  §3.3  Full_MoveOnly       — 只有移动构造和移动赋值
// ---------------------------------------------------------------------------
struct Full_MoveOnly
{
    Full_MoveOnly(Full_MoveOnly&&) {}
    Full_MoveOnly& operator=(Full_MoveOnly&&) { return *this; }
};


// ============================================================================
//  编译期验证: 构造测试
// ============================================================================

inline void verifyCtorTraits()
{
    // 用宏统一验证, 避免重复书写 7 个 static_assert
#define VERIFY_CTOR(T, exp_copy, exp_move, exp_T_ref, exp_T_rref, exp_const_T_ref) \
    do { \
        static_assert(std::is_copy_constructible<T>::value == exp_copy, \
            "is_copy_constructible<" #T "> != " #exp_copy); \
        static_assert(std::is_move_constructible<T>::value == exp_move, \
            "is_move_constructible<" #T "> != " #exp_move); \
        static_assert(std::is_constructible<T, T&>::value == exp_T_ref, \
            "is_constructible<" #T ", T&> != " #exp_T_ref); \
        static_assert(std::is_constructible<T, T&&>::value == exp_T_rref, \
            "is_constructible<" #T ", T&&> != " #exp_T_rref); \
        static_assert(std::is_constructible<T, const T&>::value == exp_const_T_ref, \
            "is_constructible<" #T ", const T&> != " #exp_const_T_ref); \
        static_assert(type::EachLvalueConstructibleChecker<T>::value == exp_T_ref, \
            "EachLvalueConstructibleChecker<" #T "> != " #exp_T_ref); \
        static_assert(type::EachRvalueConstructibleChecker<T>::value == exp_move, \
            "EachRvalueConstructibleChecker<" #T "> != " #exp_move); \
    } while(0)

    _MEIDO_INFO_RAW("\n========== 构造测试 ==========");

    //                                    copy  move  T&    T&&   constT&
    VERIFY_CTOR(Ctor_Auto,               true, true, true, true, true);
    _MEIDO_INFO_RAW("  §1.1 Ctor_Auto");
    _MEIDO_INFO_RAW("      编译器自动生成拷贝/移动构造 → 全部为 true (基线对照)");

    // ⚠️ 陷阱①: 未声明移动构造, 但 is_move_constructible=true
    VERIFY_CTOR(Ctor_CopyOnly,           true, true, true, true, true);
    _MEIDO_INFO_RAW("  §1.2 Ctor_CopyOnly");
    _MEIDO_INFO_RAW("      is_copy_constructible = true");
    _MEIDO_INFO_RAW("      is_move_constructible = true  ⚠️");
    _MEIDO_INFO_RAW("      ⇒ 只有 const T& 构造, 未声明 T(T&&)");
    _MEIDO_INFO_RAW("        T&& 表达式绑定到 const T& → move 结果为 true");
    _MEIDO_INFO_RAW("        (但类型实际没有移动构造函数, 只是通过拷贝构造'伪移动')");

    // 有 T(T&&) = delete → is_move_constructible=false (与 §1.2 不同!)
    VERIFY_CTOR(Ctor_Copy_DeleteMove,    true, false, true, false, true);
    _MEIDO_INFO_RAW("  §1.3 Ctor_Copy_DeleteMove");
    _MEIDO_INFO_RAW("      is_copy_constructible = true");
    _MEIDO_INFO_RAW("      is_move_constructible = false  ⚠️ 对比 §1.2");
    _MEIDO_INFO_RAW("      ⇒ const T& 构造 + T(T&&)=delete");
    _MEIDO_INFO_RAW("        T&& 完美匹配被 delete 的 T(T&&), 不会 fallback 到 const T&");
    _MEIDO_INFO_RAW("        所以 move 结果为 false (与 §1.2 不同!)");

    VERIFY_CTOR(Ctor_Move_DeleteCopy,    false, true, false, true, false);
    _MEIDO_INFO_RAW("  §1.4 Ctor_Move_DeleteCopy");
    _MEIDO_INFO_RAW("      is_copy_constructible = false   (const T& 被 delete)");
    _MEIDO_INFO_RAW("      is_move_constructible = true    (有 T&& 构造)");

    VERIFY_CTOR(Ctor_None,               false, false, false, false, false);
    _MEIDO_INFO_RAW("  §1.5 Ctor_None");
    _MEIDO_INFO_RAW("      拷贝和移动构造均被 delete → 全部为 false");

    VERIFY_CTOR(Ctor_Both,               true,  true,  true,  true,  true);
    _MEIDO_INFO_RAW("  §1.6 Ctor_Both");
    _MEIDO_INFO_RAW("      拷贝和移动构造均明确定义 → 全部为 true (基线对照)");

    VERIFY_CTOR(Ctor_MoveOnly,           false, true, false, true, false);
    _MEIDO_INFO_RAW("  §1.7 Ctor_MoveOnly");
    _MEIDO_INFO_RAW("      is_copy_constructible = false   (移动构造抑制了隐式拷贝)");
    _MEIDO_INFO_RAW("      is_move_constructible = true");

    // ⚠️ 陷阱③: is_copy_constructible 检查 const T&, 不是 T&
    VERIFY_CTOR(Ctor_NonConstCopy,       false, false, true,  false, false);
    _MEIDO_INFO_RAW("  §1.8 Ctor_NonConstCopy");
    _MEIDO_INFO_RAW("      is_copy_constructible = false  ⚠️");
    _MEIDO_INFO_RAW("      is_constructible<T,T&>  = true");
    _MEIDO_INFO_RAW("      ⇒ 类型只有 T(T&) 非 const 拷贝构造");
    _MEIDO_INFO_RAW("        is_copy_constructible 检查的是 const T&, 不是 T&");
    _MEIDO_INFO_RAW("        所以 copy=false, 但 is_constructible<T,T&>=true");

    VERIFY_CTOR(Ctor_Default,            true,  true,  true,  true,  true);
    _MEIDO_INFO_RAW("  §1.9 Ctor_Default");
    _MEIDO_INFO_RAW("      只声明了默认构造, 编译器自动生成 → 全部为 true");

#undef VERIFY_CTOR

    _MEIDO_INFO_RAW("[OK] 构造 static_assert 全部通过");
}


// ============================================================================
//  编译期验证: 赋值测试
// ============================================================================

inline void verifyAssignTraits()
{
#define VERIFY_ASSIGN(T, exp_copy, exp_move, exp_T_ref, exp_T_rref, exp_const_T_ref) \
    do { \
        static_assert(std::is_copy_assignable<T>::value == exp_copy, \
            "is_copy_assignable<" #T "> != " #exp_copy); \
        static_assert(std::is_move_assignable<T>::value == exp_move, \
            "is_move_assignable<" #T "> != " #exp_move); \
        static_assert(std::is_assignable<T&, T&>::value == exp_T_ref, \
            "is_assignable<" #T "&, T&> != " #exp_T_ref); \
        static_assert(std::is_assignable<T&, T&&>::value == exp_T_rref, \
            "is_assignable<" #T "&, T&&> != " #exp_T_rref); \
        static_assert(std::is_assignable<T&, const T&>::value == exp_const_T_ref, \
            "is_assignable<" #T "&, const T&> != " #exp_const_T_ref); \
    } while(0)

    _MEIDO_INFO_RAW("\n========== 赋值测试 ==========");

    // §2.1: 无移动赋值声明 → is_move_assignable=true (陷阱① 的赋值版本)
    VERIFY_ASSIGN(Assign_CopyOnly,          true, true, true, true, true);
    _MEIDO_INFO_RAW("  §2.1 Assign_CopyOnly");
    _MEIDO_INFO_RAW("      is_copy_assignable = true");
    _MEIDO_INFO_RAW("      is_move_assignable = true  ⚠️");
    _MEIDO_INFO_RAW("      ⇒ 只有 const T& 赋值, 未声明 T&& operator=");
    _MEIDO_INFO_RAW("        T&& 表达式绑定到 const T& → move 结果为 true");

    // §2.2: T&&=delete → is_move_assignable=false (陷阱② 的赋值版本)
    VERIFY_ASSIGN(Assign_Copy_DeleteMove,   true, false, true, false, true);
    _MEIDO_INFO_RAW("  §2.2 Assign_Copy_DeleteMove");
    _MEIDO_INFO_RAW("      is_copy_assignable = true");
    _MEIDO_INFO_RAW("      is_move_assignable = false  ⚠️ 对比 §2.1");
    _MEIDO_INFO_RAW("      ⇒ 有 T&& operator= =delete, 完美匹配后不会 fallback");

    VERIFY_ASSIGN(Assign_Move_DeleteCopy,   false, true, false, true, false);
    _MEIDO_INFO_RAW("  §2.3 Assign_Move_DeleteCopy");
    _MEIDO_INFO_RAW("      is_copy_assignable = false");
    _MEIDO_INFO_RAW("      is_move_assignable = true");

    VERIFY_ASSIGN(Assign_None,              false, false, false, false, false);
    _MEIDO_INFO_RAW("  §2.4 Assign_None");
    _MEIDO_INFO_RAW("      所有赋值操作符被 delete → 全部为 false");

    // §2.5: 只有 T& 赋值 → is_copy_assignable=false (陷阱③ 的赋值版本)
    //
    // ⚠️ MSVC 编译器缺陷说明:
    //   标准 C++: T&& 不能绑定到 T& (非 const 左值引用)
    //   → is_move_assignable = false, is_assignable<T&, T&&> = false
    //
    //   MSVC __is_assignable 内建有已知缺陷:
    //   当类型只有 operator=(T&) 时, 其内部错误地将 T&& 视为可绑定到 T&,
    //   导致 is_move_assignable 和 is_assignable<T&, T&&> 返回 true.
    //   这是 MSVC 编译器的缺陷, 非本库问题.
    //   参考: https://developercommunity.visualstudio.com/t/is_assignable-incorrectly-returns-true-fo/1342237
    //
#ifdef _MSC_VER
    static_assert(std::is_copy_assignable<Assign_NonConstCopy>::value == false,
        "is_copy_assignable<Assign_NonConstCopy> != false");
    static_assert(std::is_move_assignable<Assign_NonConstCopy>::value == true,
        "is_move_assignable<Assign_NonConstCopy> != true  [MSVC __is_assignable quirk]");
    static_assert(std::is_assignable<Assign_NonConstCopy&, Assign_NonConstCopy&>::value == true,
        "is_assignable<Assign_NonConstCopy&, T&> != true");
    static_assert(std::is_assignable<Assign_NonConstCopy&, Assign_NonConstCopy&&>::value == true,
        "is_assignable<Assign_NonConstCopy&, T&&> != true  [MSVC __is_assignable quirk]");
    static_assert(std::is_assignable<Assign_NonConstCopy&, const Assign_NonConstCopy&>::value == false,
        "is_assignable<Assign_NonConstCopy&, const T&> != false");
    _MEIDO_INFO_RAW("  §2.5 Assign_NonConstCopy (MSVC)");
    _MEIDO_INFO_RAW("      is_copy_assignable = false  ⚠️");
    _MEIDO_INFO_RAW("      is_assignable<T&,T&> = true");
    _MEIDO_INFO_RAW("      is_move_assignable = true   ⚠️ [MSVC __is_assignable 缺陷]");
    _MEIDO_INFO_RAW("      is_assignable<T&,T&&> = true ⚠️ [MSVC __is_assignable 缺陷]");
    _MEIDO_INFO_RAW("      ⇒ MSVC 错误地将 T&& 视为可绑定到 T&");
    _MEIDO_INFO_RAW("      ⇒ is_copy_assignable 检查的是 const T&, 不是 T&");
#else
    VERIFY_ASSIGN(Assign_NonConstCopy,      false, false, true,  false, false);
    _MEIDO_INFO_RAW("  §2.5 Assign_NonConstCopy");
    _MEIDO_INFO_RAW("      is_copy_assignable = false  ⚠️");
    _MEIDO_INFO_RAW("      is_assignable<T&,T&> = true");
    _MEIDO_INFO_RAW("      ⇒ is_copy_assignable 检查的是 const T&, 不是 T&");
#endif

    // 额外 inline 对照类型
    struct Assign_Both {
        Assign_Both& operator=(const Assign_Both&) { return *this; }
        Assign_Both& operator=(Assign_Both&&) { return *this; }
    };
    VERIFY_ASSIGN(Assign_Both,              true,  true,  true,  true,  true);
    _MEIDO_INFO_RAW("  [Ref] Assign_Both");
    _MEIDO_INFO_RAW("      拷贝和移动赋值均定义 → 全部为 true (基线对照)");

#undef VERIFY_ASSIGN

    _MEIDO_INFO_RAW("[OK] 赋值 static_assert 全部通过");
}


// ============================================================================
//  编译期验证: 综合类型测试
// ============================================================================

inline void verifyFullTraits()
{
    _MEIDO_INFO_RAW("\n========== 综合类型测试 ==========");

    // §3.1
    static_assert(std::is_copy_constructible<Full_CopyOnly_DeleteMove>::value == true, "");
    static_assert(std::is_move_constructible<Full_CopyOnly_DeleteMove>::value == false, "");
    static_assert(std::is_copy_assignable<Full_CopyOnly_DeleteMove>::value == true, "");
    static_assert(std::is_move_assignable<Full_CopyOnly_DeleteMove>::value == false, "");
    _MEIDO_INFO_RAW("  §3.1 Full_CopyOnly_DeleteMove");
    _MEIDO_INFO_RAW("      构造: is_copy_constructible=true, is_move_constructible=false  ⚠️");
    _MEIDO_INFO_RAW("      赋值: is_copy_assignable=true,  is_move_assignable=false");
    _MEIDO_INFO_RAW("      ⇒ 同时 delete 了移动构造和移动赋值, 两者 move 均为 false");

    // §3.2
    static_assert(std::is_copy_constructible<Full_Both>::value == true, "");
    static_assert(std::is_move_constructible<Full_Both>::value == true, "");
    static_assert(std::is_copy_assignable<Full_Both>::value == true, "");
    static_assert(std::is_move_assignable<Full_Both>::value == true, "");
    _MEIDO_INFO_RAW("  §3.2 Full_Both");
    _MEIDO_INFO_RAW("      构造和赋值均定义 → 全部为 true (基线对照)");

    // §3.3
    static_assert(std::is_copy_constructible<Full_MoveOnly>::value == false, "");
    static_assert(std::is_move_constructible<Full_MoveOnly>::value == true, "");
    static_assert(std::is_copy_assignable<Full_MoveOnly>::value == false, "");
    static_assert(std::is_move_assignable<Full_MoveOnly>::value == true, "");
    _MEIDO_INFO_RAW("  §3.3 Full_MoveOnly");
    _MEIDO_INFO_RAW("      构造: is_copy_constructible=false, is_move_constructible=true");
    _MEIDO_INFO_RAW("      赋值: is_copy_assignable=false,  is_move_assignable=true");

    _MEIDO_INFO_RAW("[OK] 综合类型 static_assert 全部通过");
}


// ============================================================================
//  运行时结果输出 (对照表)
// ============================================================================

inline void printCtorTable()
{
    auto b = [](bool v) -> const char* { return v ? " T " : " F "; };
    auto leftPad = [](std::string s, int w) -> std::string {
        if ((int)s.size() >= w) return s;
        return s + std::string(w - s.size(), ' ');
    };

#define PRINT_CTOR_ROW(T, note) \
    do { \
        _MEIDO_INFO_RAW(std::string("  ") \
            + leftPad(#T, 28) + " | " \
            + b(std::is_copy_constructible<T>::value) + "  | " \
            + b(std::is_move_constructible<T>::value) + "  | " \
            + b(std::is_constructible<T, T&>::value) + "  | " \
            + b(std::is_constructible<T, T&&>::value) + "  | " \
            + b(std::is_constructible<T, const T&>::value) + "  | " \
            + b(type::EachLvalueConstructibleChecker<T>::value) + "   | " \
            + b(type::EachRvalueConstructibleChecker<T>::value) + "    " \
            + note); \
    } while(0)

    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("==================== 构造测试对照表 ====================");
    _MEIDO_INFO_RAW(std::string("  ") + leftPad("类型", 28) + " | copy | move | T&  | T&& |constT&| Lval| Rval");
    _MEIDO_INFO_RAW(std::string("  ") + std::string(75, '-'));
    PRINT_CTOR_ROW(Ctor_Auto,             "");
    PRINT_CTOR_ROW(Ctor_CopyOnly,         "  ⚠️ 无移动构造声明, 但 move=T");
    PRINT_CTOR_ROW(Ctor_Copy_DeleteMove,  "  ⚠️ 有 T&&=delete, move=F (对比上行)");
    PRINT_CTOR_ROW(Ctor_Move_DeleteCopy,  "");
    PRINT_CTOR_ROW(Ctor_None,             "");
    PRINT_CTOR_ROW(Ctor_Both,             "");
    PRINT_CTOR_ROW(Ctor_MoveOnly,         "");
    PRINT_CTOR_ROW(Ctor_NonConstCopy,     "  ⚠️ is_copy_constructible 不是检查 T&");
    PRINT_CTOR_ROW(Ctor_Default,          "");

    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  注: Lval = EachLvalueConstructibleChecker (is_constructible<T, T&>)");
    _MEIDO_INFO_RAW("       Rval = EachRvalueConstructibleChecker (is_constructible<T, T&&  >)");
    _MEIDO_INFO_RAW("       ⚠️  = 反直觉/需要特别注意的结果");
    _MEIDO_INFO_RAW("");

#undef PRINT_CTOR_ROW
}

inline void printAssignTable()
{
    auto b = [](bool v) -> const char* { return v ? " T " : " F "; };
    auto leftPad = [](std::string s, int w) -> std::string {
        if ((int)s.size() >= w) return s;
        return s + std::string(w - s.size(), ' ');
    };

#define PRINT_ASSIGN_ROW(T, note) \
    do { \
        _MEIDO_INFO_RAW(std::string("  ") \
            + leftPad(#T, 28) + " | " \
            + b(std::is_copy_assignable<T>::value) + "  | " \
            + b(std::is_move_assignable<T>::value) + "  | " \
            + b(std::is_assignable<T&, T&>::value) + "  | " \
            + b(std::is_assignable<T&, T&&>::value) + "  | " \
            + b(std::is_assignable<T&, const T&>::value) + "  " \
            + note); \
    } while(0)

    _MEIDO_INFO_RAW("==================== 赋值测试对照表 ====================");
    _MEIDO_INFO_RAW(std::string("  ") + leftPad("类型", 28) + " | copy | move | T&  | T&& |constT&");
    _MEIDO_INFO_RAW(std::string("  ") + std::string(60, '-'));
    PRINT_ASSIGN_ROW(Assign_CopyOnly,          "  ⚠️ 无移动赋值声明, 但 move=T");
    PRINT_ASSIGN_ROW(Assign_Copy_DeleteMove,   "  ⚠️ 有 T&&=delete, move=F (对比上行)");
    PRINT_ASSIGN_ROW(Assign_Move_DeleteCopy,   "");
    PRINT_ASSIGN_ROW(Assign_None,              "");
    PRINT_ASSIGN_ROW(Assign_NonConstCopy,      "  ⚠️ is_copy_assignable 不是检查 T&");

    struct Assign_Both {
        Assign_Both& operator=(const Assign_Both&) { return *this; }
        Assign_Both& operator=(Assign_Both&&) { return *this; }
    };
    PRINT_ASSIGN_ROW(Assign_Both,              "");

    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  注: copy = is_copy_assignable<T> (= is_assignable<T&, const T&>)");
    _MEIDO_INFO_RAW("       move = is_move_assignable<T> (= is_assignable<T&, T&&>)");
    _MEIDO_INFO_RAW("       ⚠️  = 反直觉/需要特别注意的结果");
    _MEIDO_INFO_RAW("");

#undef PRINT_ASSIGN_ROW
}


// ============================================================================
//  §4  歧义性根源分析 — 总结速查
// ============================================================================

inline void printAnalysis()
{
    _MEIDO_INFO_RAW("==================== §4 歧义性根源分析 ====================");
    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  陷阱①  未声明 T(T&&), 但有 T(const T&)");
    _MEIDO_INFO_RAW("  ─────────────────────────────────────────────────");
    _MEIDO_INFO_RAW("  is_move_constructible = true  (× 你想检测的是'是否有移动构造')");
    _MEIDO_INFO_RAW("  is_constructible<T, T&&> = true");
    _MEIDO_INFO_RAW("  → 原因: T&& → const T& 绑定, 通过拷贝构造`伪移动`");
    _MEIDO_INFO_RAW("  案例: Ctor_CopyOnly   → move = T  ⚠️");
    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  陷阱②  声明了 T(T&&) = delete, 也有 T(const T&)");
    _MEIDO_INFO_RAW("  ─────────────────────────────────────────────────");
    _MEIDO_INFO_RAW("  is_move_constructible = false  (与陷阱①不同!)");
    _MEIDO_INFO_RAW("  → 原因: T&& 完美匹配 T(T&&) (即使 delete), 重载决议不进 const T&");
    _MEIDO_INFO_RAW("  案例: Ctor_Copy_DeleteMove → move = F");
    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  陷阱③  只有 T(T&) 非 const 左值拷贝, 无 T(const T&)");
    _MEIDO_INFO_RAW("  ─────────────────────────────────────────────────");
    _MEIDO_INFO_RAW("  is_copy_constructible = false    (× 你想检测的是'能从左值拷贝')");
    _MEIDO_INFO_RAW("  is_constructible<T, T&> = true   (这才是'非 const 左值拷贝'的检测)");
    _MEIDO_INFO_RAW("  → 原因: is_copy_xxx 实际检测的是 const T&, 不是 T&");
    _MEIDO_INFO_RAW("  案例: Ctor_NonConstCopy → copy = F, 但 T& = T  ⚠️");
    _MEIDO_INFO_RAW("");
    _MEIDO_INFO_RAW("  建议: 要精确表达意图时, 直接用 is_constructible / is_assignable");
    _MEIDO_INFO_RAW("  ─────────────────────────────────────────────────────");
    _MEIDO_INFO_RAW("  想表达'能从右值构造'          → is_constructible<T, T&&>");
    _MEIDO_INFO_RAW("  想表达'能从非 const 左值构造' → is_constructible<T, T&>");
    _MEIDO_INFO_RAW("  想表达'能从 const 左值构造'   → is_constructible<T, const T&>");
    _MEIDO_INFO_RAW("  想表达'能被右值赋值'          → is_assignable<T&, T&&>");
    _MEIDO_INFO_RAW("  想表达'能被非 const 左值赋值' → is_assignable<T&, T&>");
    _MEIDO_INFO_RAW("  想表达'能被 const 左值赋值'   → is_assignable<T&, const T&>");
    _MEIDO_INFO_RAW("");
}


// ============================================================================
//  主入口
// ============================================================================

inline void check()
{
    _MEIDO_INFO_RAW("\n"
        "████████████████████████████████████████████████████████████████████████\n"
        "█               type_traits 歧义性验证测试                           █\n"
        "█   检查 is_move_xxx / is_copy_xxx 的真实行为 vs 直觉                █\n"
        "████████████████████████████████████████████████████████████████████████");

    verifyCtorTraits();
    verifyAssignTraits();
    verifyFullTraits();

    printCtorTable();
    printAssignTable();
    printAnalysis();

    _MEIDO_INFO_RAW("████████████████ 歧义性验证测试结束 ████████████████████████████");
    _MEIDO_INFO_RAW("");
}

}    // namespace _meidotypeambiguitycheck

#endif    // !TYPE_TRAITS_AMBIGUITY_TEST_HPP_BOKUMEIDOCPP

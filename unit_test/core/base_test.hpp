/*  base 模块单元测试
    分类：版本信息验证 | 编译器宏诊断  */
#pragma once
#ifndef BASE_TEST_HPP_BOKUMEIDOCPP
#define BASE_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/base.hpp"

using namespace meido;
namespace _meidobasecheck
{

inline void getVersionTest()
{
    // 优先级1: constexpr 编译期检查
    static_assert(base::getVersion()[0] != '\0', "assert failed!");

    // 优先级2: 运行时验证版本字符串格式
    const char* ver = base::getVersion();
    MEIDO_ASSERT(ver != nullptr);

    // 验证前缀 "bokumeidocpp-"
    const char prefix[] = "bokumeidocpp-";
    MEIDO_ASSERT(strncmp(ver, prefix, sizeof(prefix) - 1) == 0);

    // 验证前缀之后有版本号内容（非空）
    const char* ver_part = ver + sizeof(prefix) - 1;
    MEIDO_ASSERT(ver_part[0] != '\0');

    // 验证包含版本分隔符 "."
    const char* dot_pos = strchr(ver_part, '.');
    MEIDO_ASSERT(dot_pos != nullptr);

    // 验证包含日期版本分隔符 "-"
    const char* dash_pos = strchr(ver_part, '-');
    MEIDO_ASSERT(dash_pos != nullptr);

    // 验证版本字符串长度合理（至少 "bokumeidocpp-X.Y.Z-"）
    size_t len = strlen(ver);
    MEIDO_ASSERT(len > sizeof(prefix) + 5);
}

inline void logVersionTest()
{
    // 调用 logVersion，验证不同入参均不崩溃
    base::logVersion();                 // 无 project_name
    base::logVersion("bokumeido_unit_test"); // 有 project_name
    base::logVersion(nullptr);          // nullptr
}

inline void funcSigTest()
{
    // MEIDO_FUNCSIG 编译期展开，验证运行时非空
    MEIDO_ASSERT(MEIDO_FUNCSIG != nullptr);
    MEIDO_ASSERT(strlen(MEIDO_FUNCSIG) > 0);

    // MEIDO_FUNCNAME 依赖 MEIDO_FUNCSIG，验证非空
    MEIDO_ASSERT(MEIDO_FUNCNAME != nullptr);
    MEIDO_ASSERT(strlen(MEIDO_FUNCNAME) > 0);

    // 以下为编译器宏展开的诊断信息，无固定预期值
    char funcsig_buf[256];
    snprintf(funcsig_buf, sizeof(funcsig_buf), "Author check! MEIDO_FUNCSIG (compiler macro) = %s", MEIDO_FUNCSIG);
    _MEIDO_INFO_RAW(funcsig_buf);
    char funcname_buf[256];
    snprintf(funcname_buf, sizeof(funcname_buf), "Author check! MEIDO_FUNCNAME (compiler macro) = %s", MEIDO_FUNCNAME);
    _MEIDO_INFO_RAW(funcname_buf);
}

// ========== makeScopeGuard 编译期约束测试（SFINAE 辅助 trait） ==========
// C++11 兼容的 void_t 实现
template <typename...>
struct ScopeGuardVoidImpl { using type = void; };
template <typename... Ts>
using ScopeGuardVoidT = typename ScopeGuardVoidImpl<Ts...>::type;

// SFINAE trait：检测 makeScopeGuard 是否对类型 T 可用
// 注意：引用类型作为参数传入时会被按值传递 decay，因此 makeScopeGuard 对引用类型实际上可用
template <typename T, typename = void>
struct is_make_scope_guardable : std::false_type {};

template <typename T>
struct is_make_scope_guardable<T, ScopeGuardVoidT<decltype(base::makeScopeGuard(std::declval<T>()))>> : std::true_type {};

// 不可移动构造的可调用类型（测试 makeScopeGuard 的移动构造约束）
struct NonMoveCallable
{
    NonMoveCallable() = default;
    NonMoveCallable(NonMoveCallable&&) = delete;
    NonMoveCallable(const NonMoveCallable&) = default;
    NonMoveCallable& operator=(NonMoveCallable&&) = delete;
    void operator()() {}
};

// 函数指针测试用函数
inline void msg_func_set_one() {}

// 运行时测试辅助——函数对象
struct MakeScopeGuardFuncObj
{
    int* counter;
    void operator()() { if (counter) ++(*counter); }
};

// 运行时测试辅助——带 LIFO 顺序的函数对象
struct MakeScopeGuardOrderObj
{
    int* val;
    int digit;
    void operator()() { if (val) *val = *val * 10 + digit; }
};

inline void makeScopeGuardStaticAssertTest()
{
    // ---- 正面测试：期望 makeScopeGuard 对以下类型可用 ----

    // lambda 类型
    auto lambda = []() {};
    static_assert(is_make_scope_guardable<decltype(lambda)>::value,
                  "lambda should be makeScopeGuardable");

    // 函数指针类型
    static_assert(is_make_scope_guardable<void(*)()>::value,
                  "function pointer should be makeScopeGuardable");

    // 可移动构造的函数对象
    struct Functor
    {
        void operator()() {}
    };
    static_assert(is_make_scope_guardable<Functor>::value,
                  "movable functor should be makeScopeGuardable");

    // 函数引用类型（按值传递 decay 为函数指针，因此可用）
    static_assert(is_make_scope_guardable<void(&)()>::value,
                  "function reference decays to pointer, should be makeScopeGuardable");

    // ---- 负面测试：期望 makeScopeGuard 对以下类型不可用 ----

    // 不可调用类型（int 无 operator()）
    static_assert(!is_make_scope_guardable<int>::value,
                  "int should NOT be makeScopeGuardable");

    // 不可移动构造的可调用类型
    static_assert(!is_make_scope_guardable<NonMoveCallable>::value,
                  "non-movable callable should NOT be makeScopeGuardable");

    // 不可移动且不可调用的类型
    static_assert(!is_make_scope_guardable<std::string>::value,
                  "non-callable type should NOT be makeScopeGuardable");
}

inline void makeScopeGuardRuntimeTest()
{
    // ---- 1. 函数指针基本功能 ----
    {
        auto guard = base::makeScopeGuard(msg_func_set_one);
        // 无运行时验证，仅确认编译通过即可
    }

    // ---- 2. 函数对象基本功能 ----
    int count = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&count});
        MEIDO_ASSERT(count == 0);
    }
    MEIDO_ASSERT(count == 1);

    // ---- 3. 多个 guard 的析构顺序（LIFO） ----
    int order_val = 0;
    {
        auto g1 = base::makeScopeGuard(MakeScopeGuardOrderObj{&order_val, 1});
        auto g2 = base::makeScopeGuard(MakeScopeGuardOrderObj{&order_val, 2});
        MEIDO_ASSERT(order_val == 0);
    }
    // g2 先析构，得 2；g1 后析构，得 21
    MEIDO_ASSERT(order_val == 21);

    // ---- 4. 连续使用同一类型构造 guard ----
    int c1 = 0, c2 = 0;
    {
        auto g1 = base::makeScopeGuard(MakeScopeGuardFuncObj{&c1});
        auto g2 = base::makeScopeGuard(MakeScopeGuardFuncObj{&c2});
        MEIDO_ASSERT(c1 == 0 && c2 == 0);
    }
    MEIDO_ASSERT(c1 == 1 && c2 == 1);
}

inline void makeScopeGuardControlTest()
{
    // ---- 1. dismiss：取消执行，析构时不触发 ----
    int count = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&count});
        guard.dismiss();
    }
    MEIDO_ASSERT(count == 0);

    // ---- 2. reset：提前执行并取消，析构时不重复执行 ----
    int val = 0;
    {
        auto guard = base::makeScopeGuard([&] { val = 42; });
        guard.reset();
        MEIDO_ASSERT(val == 42);        // reset 时立即执行
        val = 0;                        // 重置以验证析构时不会再次执行
    }
    MEIDO_ASSERT(val == 0);              // 析构时未重复执行

    // ---- 3. 重复 dismiss 安全 ----
    int c3 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c3});
        guard.dismiss();
        guard.dismiss();                // 第二次 dismiss 应无影响
    }
    MEIDO_ASSERT(c3 == 0);

    // ---- 4. 已 reset 后再 dismiss 安全 ----
    int c4 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c4});
        guard.reset();                  // 提前执行，count++
        guard.dismiss();                // 已无效，再 dismiss 无影响
    }
    MEIDO_ASSERT(c4 == 1);               // 仅 reset 时执行一次

    // ---- 5. 已 dismiss 后再 reset 无效果 ----
    int c5 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c5});
        guard.dismiss();                // 取消
        guard.reset();                  // 已取消，reset 不应执行
    }
    MEIDO_ASSERT(c5 == 0);               // 从未执行
}

inline void makeScopeGuardMoveTest()
{
    // ---- 1. 通过 std::move 传入左值，验证移动构造路径 ----
    int count = 0;
    {
        MakeScopeGuardFuncObj func_obj{&count};
        auto guard = base::makeScopeGuard(std::move(func_obj));
    }
    MEIDO_ASSERT(count == 1);

    // ---- 2. 仅移动类型（持有 unique_ptr 的函数对象）----
    struct MoveOnlyCallable
    {
        std::unique_ptr<int> data;
        int* flag;

        MoveOnlyCallable(std::unique_ptr<int> d, int* f)
            : data(std::move(d)), flag(f) {}
        MoveOnlyCallable(MoveOnlyCallable&&) = default;
        MoveOnlyCallable(const MoveOnlyCallable&) = delete;

        void operator()()
        {
            if (data && flag)
                *flag = *data;
        }
    };

    int val = 0;
    {
        auto guard = base::makeScopeGuard(
            MoveOnlyCallable(std::unique_ptr<int>(new int(99)), &val));
    }
    MEIDO_ASSERT(val == 99);

    // ---- 3. 仅移动类型 + std::move 传入左值 ----
    int val2 = 0;
    {
        MoveOnlyCallable moc(std::unique_ptr<int>(new int(88)), &val2);
        auto guard = base::makeScopeGuard(std::move(moc));
    }
    MEIDO_ASSERT(val2 == 88);
}

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check base start--------------------");

    // ---- 1. 版本信息验证 ----
    getVersionTest();
    logVersionTest();

    // ---- 2. 编译器宏诊断 ----
    funcSigTest();

    // ---- 3. makeScopeGuard 编译期约束测试 ----
    makeScopeGuardStaticAssertTest();

    // ---- 4. makeScopeGuard 运行时功能测试 ----
    makeScopeGuardRuntimeTest();

    // ---- 5. ScopeGuard dismiss/reset 控制接口测试 ----
    makeScopeGuardControlTest();

    // ---- 6. ScopeGuard 移动构造/仅移动类型测试 ----
    makeScopeGuardMoveTest();

    _MEIDO_INFO_RAW("---------------------check base end---------------------\n\n");
}

}    // namespace _meidobasecheck

#endif    // !BASE_TEST_HPP_BOKUMEIDOCPP

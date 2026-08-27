/*  base 模块单元测试
    分类：版本信息验证 | 编译器宏诊断  */
#pragma once
#ifndef BASE_TEST_HPP_BOKUMEIDOCPP
#define BASE_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/base.hpp"

using namespace meido;

// ========== MEIDO_FUNCNAME 拆分正确性测试：被测类型与函数 ==========
// 定义在全局命名空间（贴近真实使用场景）；期望值对 GCC/Clang/MSVC 一致
//（MSVC 的 "operator <<" 空格形式由 splitFuncName 归一为 operator<<）
const char* g_captured_name = nullptr;    // 记录最近一次 MEIDO_FUNCNAME 的取值

struct FuncNameProbeStruct
{
    int memValue(int)
    {
        g_captured_name = MEIDO_FUNCNAME;
        return 0;
    }
    const double& memConstRef() const
    {
        static double v = 0;
        g_captured_name = MEIDO_FUNCNAME;
        return v;
    }
    static int* memStaticPtr()
    {
        g_captured_name = MEIDO_FUNCNAME;
        return nullptr;
    }
    int& operator<<(int)
    {
        static int v = 0;
        g_captured_name = MEIDO_FUNCNAME;
        return v;
    }
    void operator()(int)
    { g_captured_name = MEIDO_FUNCNAME; }
    bool operator<(const FuncNameProbeStruct&) const
    {
        g_captured_name = MEIDO_FUNCNAME;
        return false;
    }
    operator bool() const
    {
        g_captured_name = MEIDO_FUNCNAME;
        return true;
    }
};

inline const char* probeFreeValue(int)
{ return MEIDO_FUNCNAME; }
inline const std::vector<int>& probeFreeConstRef()
{
    static std::vector<int> v;
    g_captured_name = MEIDO_FUNCNAME;
    return v;
}

inline int probeOverloaded(int)
{
    g_captured_name = MEIDO_FUNCNAME;
    return 0;
}
inline int probeOverloaded(int, int)
{
    g_captured_name = MEIDO_FUNCNAME;
    return 0;
}

template <class T>
inline const T& probeTmplConstRef(const T& v)
{
    g_captured_name = MEIDO_FUNCNAME;
    return v;
}

// ---- 扩展场景 probe：函数指针返回 / 命名空间内类 / 多参数模板类 / 嵌套模板类 ----
inline void (*probeFnPtr())(int)
{
    g_captured_name = MEIDO_FUNCNAME;
    return nullptr;
}

namespace meidoFnProbeNs
{
struct NsProbe
{
    int& getRef()
    {
        static int v = 0;
        g_captured_name = MEIDO_FUNCNAME;
        return v;
    }
};
}    // namespace meidoFnProbeNs

template <class T, class U>
struct TmplMultiProbe
{
    U pick(T& t)
    {
        g_captured_name = MEIDO_FUNCNAME;
        return t;
    }
};

template <class T>
struct NestProbe
{
    T& get()
    {
        g_captured_name = MEIDO_FUNCNAME;
        return val_;
    }
    T val_{};
};

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
    base::logVersion();                         // 无 project_name
    base::logVersion("bokumeido_unit_test");    // 有 project_name
    base::logVersion(nullptr);                  // nullptr
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

// ========== MEIDO_FUNCNAME 拆分正确性测试 ==========
// 覆盖返回类型为值/模板引用/函数指针、成员限定、运算符重载、重载函数、模板函数/类、命名空间限定、lambda 等场景，
// 各场景按解析代码路径互补去重：&/*/(&/(* 剥离走同一 find_first_not_of 路径，仅保留代表性用例
// 注意：无限定名的纯函数名用例在实现整体退化时也能通过（退化返回 __func__ 恰好等于期望），
// 解析真实性由成员/模板类/命名空间/lambda 等带限定名用例兜底

// lambda 名称兼容断言：GCC 的 sig 不含 operator() 走退化路径返回 "operator()"；
// Clang 为 foo(...)::(anonymous class)::operator()；MSVC 为 `foo'::<lambda_N>::operator ()
// 只校验去空格后以 operator() 结尾：外层函数参数表（如 (int &)）是编译器限定名的合法部分，
// 不能检查不含 '&'；返回类型符号残留的回归保护由各精确 strcmp 用例承担
inline void checkLambdaName(const char* n)
{
    std::string n_nospace = n;
    n_nospace.erase(std::remove(n_nospace.begin(), n_nospace.end(), ' '), n_nospace.end());
    MEIDO_ASSERT(n_nospace.size() >= strlen("operator()")
                 && n_nospace.compare(n_nospace.size() - strlen("operator()"), strlen("operator()"), "operator()") == 0);
}

// lambda 外层函数带多参数/引用/复杂类型参数时，函数参数表 (int, int) / (const std::string&) 内含空格，
// 解析不得被这些空格提前截断
inline void meidoFuncNameMultiArgOuter(int a, int b)
{
    auto multi_lambda = [](int x, int y) { g_captured_name = MEIDO_FUNCNAME; (void)x; (void)y; };
    multi_lambda(a, b);
}

inline void meidoFuncNameComplexOuter(int& a, int b, const std::string& s)
{
    (void)a;
    (void)b;
    (void)s;
    auto complex_lambda = []() { g_captured_name = MEIDO_FUNCNAME; };
    complex_lambda();
}

inline void meidoFuncNameTest()
{
    // ---- 自由函数：值 / 常量引用（模板返回类型 + &）返回 ----
    MEIDO_ASSERT(strcmp(probeFreeValue(1), "probeFreeValue") == 0);
    probeFreeConstRef();
    MEIDO_ASSERT(strcmp(g_captured_name, "probeFreeConstRef") == 0);

    // ---- 成员函数：const 限定 + 引用返回 / 静态 + 指针返回 ----
    FuncNameProbeStruct ms;
    ms.memValue(1);
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::memValue") == 0);
    ms.memConstRef();
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::memConstRef") == 0);
    FuncNameProbeStruct::memStaticPtr();
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::memStaticPtr") == 0);

    // ---- 运算符重载：名字按编译器原样保留（MSVC 的 __func__ 为 "operator <<" 带空格，GCC/Clang 为 "operator<<"）----
    // 不统一格式，只验证拆分正确（不含返回类型符号、限定名完整）
#if defined(__clang_cl__)
    // clang-cl（依据 clang 源码推断）：__FUNCSIG__ 的函数名部分用 clang 的 printQualifiedName 打印（无空格），
    // 与 __func__（clang 前端，无空格）一致，find 命中走正常解析；仍需 clang-cl 实测确认
    const char* ms_op_shift = "FuncNameProbeStruct::operator<<";
    const char* ms_op_call = "FuncNameProbeStruct::operator()";
#elif defined(_MEIDO_MSVC_LIKE)
    const char* ms_op_shift = "FuncNameProbeStruct::operator <<";
    const char* ms_op_call = "FuncNameProbeStruct::operator ()";
#else
    const char* ms_op_shift = "FuncNameProbeStruct::operator<<";
    const char* ms_op_call = "FuncNameProbeStruct::operator()";
#endif
    ms << 3;
    MEIDO_ASSERT(strcmp(g_captured_name, ms_op_shift) == 0);
    ms(7);
    MEIDO_ASSERT(strcmp(g_captured_name, ms_op_call) == 0);
    bool b = ms;
    (void)b;
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::operator bool") == 0);    // 转换运算符名字中的空格合法

    // ---- 重载函数（不同签名解析出同名）----
    probeOverloaded(1, 2);
    MEIDO_ASSERT(strcmp(g_captured_name, "probeOverloaded") == 0);

    // ---- 模板函数（const T& 返回）----
    std::string s;
    probeTmplConstRef(s);
    MEIDO_ASSERT(strcmp(g_captured_name, "probeTmplConstRef") == 0);

    // ---- 函数指针返回：名字前的 '(*' 需被跳过 ----
    probeFnPtr();
    MEIDO_ASSERT(strcmp(g_captured_name, "probeFnPtr") == 0);

    // ---- 命名空间内类成员：限定名含 命名空间::类名:: ----
    meidoFnProbeNs::NsProbe ns_probe;
    ns_probe.getRef();
    MEIDO_ASSERT(strcmp(g_captured_name, "meidoFnProbeNs::NsProbe::getRef") == 0);

    // ---- 多参数模板类 + 引用模板实参（MSVC 会展开 std::string 为长类型串）：限定名含 <…, …&> ----
    std::string multi_s;
    TmplMultiProbe<std::string, std::string&> multi_probe;
    multi_probe.pick(multi_s);
    {
        const std::string& n = g_captured_name;
        MEIDO_ASSERT(n.compare(0, strlen("TmplMultiProbe<"), "TmplMultiProbe<") == 0);    // 类名+模板参数完整位于名字开头
        MEIDO_ASSERT(n.size() > strlen("::pick") && n.compare(n.size() - strlen("::pick"), strlen("::pick"), "::pick") == 0);
    }

    // ---- 嵌套模板类限定名（>> 连续闭合）：S<std::vector<int>>::get ----
    NestProbe<std::vector<int>> nest_probe;
    nest_probe.get();
    {
        const std::string& n = g_captured_name;
        MEIDO_ASSERT(n.compare(0, strlen("NestProbe<"), "NestProbe<") == 0);
        MEIDO_ASSERT(n.size() > strlen("::get") && n.compare(n.size() - strlen("::get"), strlen("::get"), "::get") == 0);
    }

    // ---- operator<（名字内孤立的 '<' 不能误判为模板参数起始）----
    ms < ms;
#if defined(__clang_cl__)
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::operator<") == 0);    // 同 clang-cl 运算符块注释
#elif defined(_MEIDO_MSVC_LIKE)
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::operator <") == 0);
#else
    MEIDO_ASSERT(strcmp(g_captured_name, "FuncNameProbeStruct::operator<") == 0);
#endif

    // ---- lambda：GCC sig 不含 operator() 走退化路径；Clang/MSVC 完整解析，做兼容断言 ----
    int a = 5;
    auto probe_lambda = []() { g_captured_name = MEIDO_FUNCNAME; };
    probe_lambda();
    checkLambdaName(g_captured_name);

    // ---- lambda 外层函数带多参数/引用/复杂类型参数：限定名中函数参数表含空格，不得提前截断 ----
    meidoFuncNameMultiArgOuter(a, 2);
    checkLambdaName(g_captured_name);
#if defined(__clang__) && !defined(_MEIDO_MSVC_LIKE)
    // Clang 可完整解析 lambda 限定名：必须包含外层函数名（截断 bug 会从参数表处截掉函数名）
    MEIDO_ASSERT(strstr(g_captured_name, "meidoFuncNameMultiArgOuter") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "(anonymous class)::operator()") != nullptr);
#elif defined(_MEIDO_MSVC_LIKE) && !defined(__clang__)
    // 纯 MSVC 实测形态：outerFunc::<lambda_hash>::operator ()，无参数表
    MEIDO_ASSERT(strstr(g_captured_name, "meidoFuncNameMultiArgOuter") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "<lambda_") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "::operator ()") != nullptr);
#endif
    meidoFuncNameComplexOuter(a, 2, s);
    checkLambdaName(g_captured_name);
#if defined(__clang__) && !defined(_MEIDO_MSVC_LIKE)
    MEIDO_ASSERT(strstr(g_captured_name, "meidoFuncNameComplexOuter") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "(anonymous class)::operator()") != nullptr);
#elif defined(_MEIDO_MSVC_LIKE) && !defined(__clang__)
    MEIDO_ASSERT(strstr(g_captured_name, "meidoFuncNameComplexOuter") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "<lambda_") != nullptr);
    MEIDO_ASSERT(strstr(g_captured_name, "::operator ()") != nullptr);
#endif
}

// ========== makeScopeGuard 编译期约束测试（SFINAE 辅助 trait） ==========
// C++11 兼容的 void_t 实现
template <typename...>
struct ScopeGuardVoidImpl
{
    using type = void;
};
template <typename... Ts>
using ScopeGuardVoidT = typename ScopeGuardVoidImpl<Ts...>::type;

// SFINAE trait：检测 makeScopeGuard 是否对类型 T 可用
// 注意：引用类型作为参数传入时会被按值传递 decay，因此 makeScopeGuard 对引用类型实际上可用
template <typename T, typename = void>
struct is_make_scope_guardable : std::false_type
{
};

template <typename T>
struct is_make_scope_guardable<T, ScopeGuardVoidT<decltype(base::makeScopeGuard(std::declval<T>()))>> : std::true_type
{
};

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
    void operator()()
    {
        if (counter) ++(*counter);
    }
};

// 运行时测试辅助——带 LIFO 顺序的函数对象
struct MakeScopeGuardOrderObj
{
    int* val;
    int digit;
    void operator()()
    {
        if (val) *val = *val * 10 + digit;
    }
};

inline void makeScopeGuardStaticAssertTest()
{
    // ---- 正面测试：期望 makeScopeGuard 对以下类型可用 ----

    // lambda 类型
    auto lambda = []() {};
    static_assert(is_make_scope_guardable<decltype(lambda)>::value,
                  "lambda should be makeScopeGuardable");

    // 函数指针类型
    static_assert(is_make_scope_guardable<void (*)()>::value,
                  "function pointer should be makeScopeGuardable");

    // 可移动构造的函数对象
    struct Functor
    {
        void operator()() {}
    };
    static_assert(is_make_scope_guardable<Functor>::value,
                  "movable functor should be makeScopeGuardable");

    // 函数引用类型（按值传递 decay 为函数指针，因此可用）
    static_assert(is_make_scope_guardable<void (&)()>::value,
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
        MEIDO_ASSERT(val == 42);    // reset 时立即执行
        val = 0;                    // 重置以验证析构时不会再次执行
    }
    MEIDO_ASSERT(val == 0);    // 析构时未重复执行

    // ---- 3. 重复 dismiss 安全 ----
    int c3 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c3});
        guard.dismiss();
        guard.dismiss();    // 第二次 dismiss 应无影响
    }
    MEIDO_ASSERT(c3 == 0);

    // ---- 4. 已 reset 后再 dismiss 安全 ----
    int c4 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c4});
        guard.reset();      // 提前执行，count++
        guard.dismiss();    // 已无效，再 dismiss 无影响
    }
    MEIDO_ASSERT(c4 == 1);    // 仅 reset 时执行一次

    // ---- 5. 已 dismiss 后再 reset 无效果 ----
    int c5 = 0;
    {
        auto guard = base::makeScopeGuard(MakeScopeGuardFuncObj{&c5});
        guard.dismiss();    // 取消
        guard.reset();      // 已取消，reset 不应执行
    }
    MEIDO_ASSERT(c5 == 0);    // 从未执行
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

        MoveOnlyCallable(std::unique_ptr<int> d, int* f) : data(std::move(d)), flag(f) {}
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
    meidoFuncNameTest();

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

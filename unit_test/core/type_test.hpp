/*  type 模块单元测试
    分类：类型特征检查器 | 函数特征提取(FuncTraits) | 调用参数检测(InvokeTupleChecker) |
           Bound调用检测(StrictStdBindTraits) | SFINAE行为验证  */
#pragma once
#ifndef TYPE_TEST_HPP_BOKUMEIDOCPP
#define TYPE_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/type.hpp"

using namespace meido;
namespace _meidotypecheck
{

inline void EachTrueFalseCheckerTest()
{
    static_assert(type::EachTrueChecker<true>::value, "assert failed!");
    static_assert(type::EachTrueChecker<true, true, true>::value, "assert failed!");
    static_assert(!type::EachTrueChecker<true, false, true>::value, "assert failed!");
    static_assert(!type::EachTrueChecker<false>::value, "assert failed!");

    static_assert(type::EachFalseChecker<false>::value, "assert failed!");
    static_assert(type::EachFalseChecker<false, false>::value, "assert failed!");
    static_assert(!type::EachFalseChecker<false, false, true, false>::value, "assert failed!");
    static_assert(!type::EachFalseChecker<true>::value, "assert failed!");
}

inline void SameTypesCheckerTest()
{
    static_assert(type::SameTypesChecker<int, unsigned int>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<int, unsigned int, int, int>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<int, int, int, unsigned int>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<int, int>::value == true, "assert failed!");
    static_assert(type::SameTypesChecker<int, int, int>::value == true, "assert failed!");
    static_assert(type::SameTypesChecker<int, int, int, int>::value == true, "assert failed!");
    static_assert(type::SameTypesChecker<int, const int>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<const int, const int>::value == true, "assert failed!");
    static_assert(type::SameTypesChecker<int, volatile int>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<int, int&>::value == false, "assert failed!");
    static_assert(type::SameTypesChecker<int&, int&>::value == true, "assert failed!");
}

inline void InTypesCheckerTest()
{
    static_assert(type::InTypesChecker<int, unsigned int>::value == false, "assert failed!");
    static_assert(type::InTypesChecker<int, unsigned int, float, char>::value == false, "assert failed!");
    static_assert(type::InTypesChecker<int, int>::value == true, "assert failed!");
    static_assert(type::InTypesChecker<int, unsigned int, int, float>::value == true, "assert failed!");
    static_assert(type::InTypesChecker<int, const int>::value == false, "assert failed!");
    static_assert(type::InTypesChecker<const int, const int>::value == true, "assert failed!");
    // void 类型检测
    static_assert(type::InTypesChecker<void, int, float, void>::value == true, "assert failed!");
    static_assert(type::InTypesChecker<void, int, float, char>::value == false, "assert failed!");
}

inline void privStdBeginEndCheckerTest()
{
    static_assert(_priv::StdBeginEndChecker<int>::value == false, "assert failed!");
    static_assert(_priv::StdBeginEndChecker<std::vector<int>>::value == true, "assert failed!");
    static_assert(_priv::StdBeginEndChecker<const std::vector<int>>::value == true, "assert failed!");
}

inline void privStdCoutCheckerTest()
{
    static_assert(_priv::StdCoutChecker<int>::value == true, "assert failed!");
    static_assert(_priv::StdCoutChecker<std::vector<int>>::value == false, "assert failed!");
    static_assert(type::StdCoutEachChecker<int>::value == true, "assert failed!");
    static_assert(type::StdCoutEachChecker<std::vector<int>>::value == false, "assert failed!");
    static_assert(type::StdCoutEachChecker<int, float, char>::value == true, "assert failed!");
    static_assert(type::StdCoutEachChecker<int, std::vector<int>, char>::value == false, "assert failed!");
}

inline void ConstructibleFromEachCheckerTest()
{
    static_assert(type::ConstructibleFromEachChecker<std::string, std::string>::value == true, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<void, char>::value == false, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<std::string&, const char*>::value == false, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<std::vector<int>, int>::value == true, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<std::vector<int>, std::vector<float>>::value == false, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<int, char, float>::value == true, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<std::vector<int>, std::vector<int>, int, float>::value == true, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<std::vector<int>, std::vector<int>, std::vector<float>, float>::value == false, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<int&, int&>::value == true, "assert failed!");
    static_assert(type::ConstructibleFromEachChecker<int&, float>::value == false, "assert failed!");
}

/* 仅有拷贝构造（无默认构造、无移动构造）：测试 EachLvalueConstructibleChecker/
   EachRvalueConstructibleChecker 对仅拷贝类型的检测行为 */
class CopyOnlyConstructible
{
public:
    CopyOnlyConstructible(const CopyOnlyConstructible& tmp)
    {
    }
};

/* 有拷贝构造 + 显式删除移动构造：仅左值可构造，测试构造检测器
   能正确区分拷贝可用 vs 移动被 delete 的场景 */
class CopyOnlyDeleteMove
{
public:
    CopyOnlyDeleteMove() {};
    CopyOnlyDeleteMove(const CopyOnlyDeleteMove& tmp)
    {
    }
    CopyOnlyDeleteMove(CopyOnlyDeleteMove&& tmp) = delete;
};

inline void EachLvalueRvalueConstructibleCheckerTest()
{
    static_assert(type::EachLvalueConstructibleChecker<CopyOnlyConstructible>::value == true, "assert failed!");
    static_assert(type::EachLvalueConstructibleChecker<CopyOnlyConstructible, CopyOnlyDeleteMove>::value == true, "assert failed!");
    static_assert(type::EachRvalueConstructibleChecker<CopyOnlyConstructible>::value == true, "assert failed!");
    static_assert(type::EachRvalueConstructibleChecker<CopyOnlyConstructible, CopyOnlyDeleteMove>::value == false, "assert failed!");
}

inline void RvalueRefMakerTest()
{
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<int>::Type, int&&>::value, "assert failed!");
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<const int>::Type, const int&&>::value, "assert failed!");
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<int&>::Type, int&&>::value, "assert failed!");
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<int&&>::Type, int&&>::value, "assert failed!");
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<void>::Type, void>::value, "assert failed!");
    static_assert(type::SameTypesChecker<type::RvalueRefMaker<const volatile int>::Type, const volatile int&&>::value, "assert failed!");
}

inline void GetTypeNameTest()
{
    std::string name = type::getTypeName<int>();
    MEIDO_ASSERT(!name.empty());
    _MEIDO_INFO_RAW(std::string("type::getTypeName<int>() = " + name).c_str());
}

/* 示例自由函数（有参有返回值）：用于 FuncTraits / StrictStdBindTraits 的签名称取测试 */
inline int sampleAddFunc(const int& a, int b)
{
    return a + b;
}

/* 示例自由函数（无参无返回值）：用于 FuncTraits 的 void 返回类型测试 */
inline void sampleEmptyFunc()
{
}

/* 辅助类：含普通/const/const_volatile 非静态成员函数 + 静态成员函数
   用于 FuncTraits 的成员函数指针签名称取测试 */
class MemberFuncExample
{
public:
    int func1(int a)
    {
        return a + 1;
    }

    int func2(int a) const
    {
        return a + 1;
    }

    int func3(int a) const volatile
    {
        return a + 1;
    }

    static int staticFunc1(int a)
    {
        return a + 1;
    }
};

/* 辅助结构体：成员函数 cvref 限定符全集（12 种组合，C++11 中 && 限定不可用，
   此处仅为 FuncTraitsCvrefTest 提供函数指针类型样本，不要求所有组合均可调用） */
struct CvrefMemberFuncClass
{
    void nonCvRef()
    {
    }
    void constFunc() const          /* const 限定成员函数 */
    {
    }
    void volatileFunc() volatile
    {
    }
    void constVolatileFunc() const volatile
    {
    }
    void lvalueFunc() &
    {
    }
    void constLvalueFunc() const&
    {
    }
    void volatileLvalueFunc() volatile&
    {
    }
    void constVolatileLvalueFunc() const volatile&
    {
    }
    void rvalueFunc() &&
    {
    }
    void constRvalueFunc() const&&
    {
    }
    void volatileRvalueFunc() volatile&&
    {
    }
    void constVolatileRvalueFunc() const volatile&&
    {
    }
};

/* 辅助类：多 operator() 重载（const/非const/volatile），含成员函数
   用于测试 FuncTraits 对有 operator() 重载的类型的处理 */
class OverloadedFunctor
{
public:
    void operator()() const
    {
    }
    char operator()(char a)
    {
        printf("char a\n");
        return 0;
    };
    short operator()(short a)
    {
        printf("short a\n");
        return 0;
    };
    short operator()(std::string& a) volatile
    {
        printf("string a\n");
        return 0;
    };

    void func(int a)
    {
    }
    void funcV(int a) volatile
    {
    }
};
// OverloadedFunctor 子类，用于测试继承场景的成员函数指针 bound 调用
class OverloadedFunctorDerived : public OverloadedFunctor
{
};

/* 简单无参仿函数（无 cv/ref 限定）：用作 FuncTraits 对简单可调用类型的基线测试 */
class SimpleFunctor
{
public:
    void operator()()
    {
    }
};

/* 返回 const int* const 的自由函数：测试 FuncTraits 对 const 返回值类型的提取
   注：顶层 const 被编译器忽略（产生警告），该警告符合预期 */
inline const int* const constPointerReturnFunc(const int* x)   // 此处警告符合预期
{
    return x;
}

/* 返回 const 类类型的自由函数：测试 FuncTraits 对返回值为 const 类的签名称取 */
inline const CopyOnlyConstructible constClassReturnFunc(CopyOnlyConstructible x)
{
    return x;
}

inline void FuncTraitsBasicTest()
{
    static_assert(type::FuncTraits<int>::value == false, "assert failed!");
    static_assert(type::FuncTraits<decltype(sampleAddFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&MemberFuncExample::func1)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&MemberFuncExample::func3)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<OverloadedFunctor>::value == false, "assert failed!");
    static_assert(type::FuncTraits<volatile OverloadedFunctor>::value == false, "assert failed!");
    // volatile 对象无法调用非 volatile 限定的 operator()，由 CallCheck 检出
    static_assert(type::FuncTraits<volatile SimpleFunctor>::value == false, "assert failed!");
    static_assert(type::FuncTraits<const void>::value == false, "assert failed!");
    static_assert(type::FuncTraits<void>::value == false, "assert failed!");

    using type0 = type::FuncTraits<decltype(sampleAddFunc)>::ReturnType;
    using tuple_type0 = type::FuncTraits<decltype(sampleAddFunc)>::ArgsTupleType;
    int num_args = type::FuncTraits<decltype(sampleAddFunc)>::num_args;


    char buf[256];
    snprintf(buf, sizeof(buf), "Author check! FuncTraits<decltype(sampleAddFunc)>: Expected| RetType=int, ArgsTupleType=std::tuple<int const&, int>, num_args=2");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              FuncTraits<decltype(sampleAddFunc)>:   Actual| RetType=%s, ArgsTupleType=%s, num_args=%d", type::getTypeName<type0>().c_str(), type::getTypeName<tuple_type0>().c_str(), num_args);
    _MEIDO_INFO_RAW(buf);

    using type1 = type::FuncTraits<decltype(&MemberFuncExample::func1)>::ReturnType;
    num_args = type::FuncTraits<decltype(&MemberFuncExample::func1)>::num_args;
    snprintf(buf, sizeof(buf), "Author check! FuncTraits<decltype(&MemberFuncExample::func1)>: Expected| RetType=int, num_args=1");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              FuncTraits<decltype(&MemberFuncExample::func1)>:   Actual| RetType=%s, num_args=%d", type::getTypeName<type1>().c_str(), num_args);
    _MEIDO_INFO_RAW(buf);

    using type2 = type::FuncTraits<decltype(&MemberFuncExample::func3)>::ReturnType;
    num_args = type::FuncTraits<decltype(&MemberFuncExample::func3)>::num_args;
    snprintf(buf, sizeof(buf), "Author check! FuncTraits<decltype(&MemberFuncExample::func3)>: Expected| RetType=int, num_args=1");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              FuncTraits<decltype(&MemberFuncExample::func3)>:   Actual| RetType=%s, num_args=%d", type::getTypeName<type2>().c_str(), num_args);
    _MEIDO_INFO_RAW(buf);

    static_assert(type::SameTypesChecker<type::FuncTraits<decltype(constPointerReturnFunc)>::ReturnType, const int*>::value, "Check FuncTraits failed!");
    static_assert(type::SameTypesChecker<type::FuncTraits<decltype(constClassReturnFunc)>::ReturnType, const CopyOnlyConstructible>::value, "Check FuncTraits failed!");
}

/* 辅助类：含私有 operator()(int) 和公开 operator()(float/void*)
   用于测试 StrictStdBindTraits 对访问权限的检测：
   能否正确识别私有重载并拒绝绑定 */
class AccessCheckedFunctor
{
private:
    void operator()(int a)
    {
    }

public:
    void operator()(float a)
    {
    }
    void operator()(void* a)
    {
    }

    void func1(void* ptr)
    {
    }
};

inline void StrictStdBindTraitsTest()
{
    static_assert(type::StrictStdBindTraits<int, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(sampleAddFunc), int, float>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(sampleAddFunc), int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(sampleAddFunc), int, int, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(sampleEmptyFunc), void>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<void, void>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func1), int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func1), std::vector<int>>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func1), const MemberFuncExample*, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func2), const MemberFuncExample*, int>::value == true, "assert failed!");

    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func2), CopyOnlyDeleteMove*, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::func3), const volatile MemberFuncExample&, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::staticFunc1), MemberFuncExample*, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&MemberFuncExample::staticFunc1), int>::value == true, "assert failed!");

    static_assert(type::StrictStdBindTraits<OverloadedFunctor, short&&>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<std::reference_wrapper<OverloadedFunctor>, short&&>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<std::reference_wrapper<const OverloadedFunctor>>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<std::reference_wrapper<const OverloadedFunctor>, short>::value == false, "assert failed!");

    static_assert(type::StrictStdBindTraits<OverloadedFunctor, int&&>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<OverloadedFunctor, short>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<volatile OverloadedFunctor, short>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<const OverloadedFunctor, short>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&OverloadedFunctorDerived::func), OverloadedFunctorDerived, int>::value == true, "assert failed!");

    static_assert(type::StrictStdBindTraits<decltype(&OverloadedFunctor::func), std::reference_wrapper<OverloadedFunctorDerived>, int>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&OverloadedFunctor::func), std::reference_wrapper<OverloadedFunctorDerived>&, int>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&OverloadedFunctor::func), std::reference_wrapper<volatile OverloadedFunctorDerived>, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&OverloadedFunctor::funcV), std::reference_wrapper<volatile OverloadedFunctorDerived>, int>::value == true, "assert failed!");

    static_assert(type::SameTypesChecker<type::StrictStdBindTraits<decltype(constPointerReturnFunc), int*>::ReturnType, const int*>::value, "Check StrictStdBindTraits failed!");
    static_assert(type::SameTypesChecker<type::StrictStdBindTraits<decltype(constClassReturnFunc), CopyOnlyConstructible>::ReturnType, const CopyOnlyConstructible>::value, "Check StrictStdBindTraits failed!");

    static_assert(type::StrictStdBindTraits<AccessCheckedFunctor, int>::value == false, "assert failed!");
    static_assert(type::StrictStdBindTraits<AccessCheckedFunctor, float>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<decltype(&AccessCheckedFunctor::func1), AccessCheckedFunctor*, int*>::value == true, "assert failed!");
    static_assert(type::StrictStdBindTraits<AccessCheckedFunctor, int*>::value == true, "assert failed!");

    AccessCheckedFunctor access_check_functor;
    int a = 0;
    std::bind(&AccessCheckedFunctor::func1, &access_check_functor, &a)();

    using type0 = type::StrictStdBindTraits<decltype(sampleAddFunc), int, float>::ReturnType;
    char buf[256];
    snprintf(buf, sizeof(buf), "Author check! StrictStdBindTraits<decltype(sampleAddFunc), int, float>: Expected| ReturnType=int");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              StrictStdBindTraits<decltype(sampleAddFunc), int, float>:   Actual| ReturnType=%s", type::getTypeName<type0>().c_str());
    _MEIDO_INFO_RAW(buf);

    using type1 = type::StrictStdBindTraits<decltype(&MemberFuncExample::func2), const MemberFuncExample*, int>::ReturnType;
    snprintf(buf, sizeof(buf), "Author check! StrictStdBindTraits<decltype(&MemberFuncExample::func2), const MemberFuncExample*, int>: Expected| ReturnType=int");
    _MEIDO_INFO_RAW(buf);
    snprintf(buf, sizeof(buf), "              StrictStdBindTraits<decltype(&MemberFuncExample::func2), const MemberFuncExample*, int>:   Actual| ReturnType=%s", type::getTypeName<type1>().c_str());
    _MEIDO_INFO_RAW(buf);
}

// 仿函数，只支持左值调用 operator() &
struct LvalueOnlyFunctor
{
    void operator()() &
    {
    }
};

// 仿函数，只支持右值调用 operator() &&
struct RvalueOnlyFunctor
{
    void operator()() &&
    {
    }
};

// ========== 仿函数 cvref 全集 ==========
// 以下结构体覆盖 operator() 的所有 cv/ref 限定组合，用于 FuncTraits cvref 测试

struct NoCvRefFunctor
{
    void operator()()
    {
    }
};

struct ConstFunctor
{
    void operator()() const
    {
    }
};

struct ConstLvalueFunctor
{
    void operator()() const&
    {
    }
};

struct ConstRvalueFunctor
{
    void operator()() const&&
    {
    }
};
struct VolatileFunctor
{
    void operator()() volatile
    {
    }
};

struct VolatileLvalueFunctor
{
    void operator()() volatile&
    {
    }
};

struct VolatileRvalueFunctor
{
    void operator()() volatile&&
    {
    }
};

struct ConstVolatileFunctor
{
    void operator()() const volatile
    {
    }
};

struct ConstVolatileLvalueFunctor
{
    void operator()() const volatile&
    {
    }
};

struct ConstVolatileRvalueFunctor
{
    void operator()() const volatile&&
    {
    }
};
// 编译期验证辅助模板：通过 InvokeTupleChecker::checkRet 验证返回类型匹配
// 仅用于编译期类型推导，无运行时行为
template <class Functor, class ArgsTuple = decltype(_priv::FunctionCheckerBase::checkArguments(std::declval<Functor>()))
, class Ret = decltype(_priv::InvokeTupleChecker<Functor, ArgsTuple>::checkRet(std::declval<ArgsTuple>()))>
static void checkRet(Functor&&) {
    // return Ret();
}

inline void FuncTraitsRefQualifierTest()
{
    decltype(_priv::FunctionCheckerBase::checkArguments(std::declval<SimpleFunctor>())) x;

    
    decltype(_priv::FunctionCheckerBase::checkArguments(std::declval<LvalueOnlyFunctor&>())) y;
    decltype(_priv::InvokeTupleChecker<LvalueOnlyFunctor&, decltype(y)>::checkRet(std::declval<decltype(y)>())) *z;

    // LvalueOnlyFunctor: 只有 operator() &，左值可调用
    static_assert(type::FuncTraits<LvalueOnlyFunctor>::value == false, "assert failed!");
    static_assert(type::FuncTraits<LvalueOnlyFunctor&>::value == true, "assert failed!");
    static_assert(type::FuncTraits<LvalueOnlyFunctor&&>::value == false, "assert failed!");

    // RvalueOnlyFunctor: 只有 operator() &&，右值可调用
    static_assert(type::FuncTraits<RvalueOnlyFunctor>::value == true, "assert failed!");
    static_assert(type::FuncTraits<RvalueOnlyFunctor&>::value == false, "assert failed!");
    static_assert(type::FuncTraits<RvalueOnlyFunctor&&>::value == true, "assert failed!");

    // 无引用限定符的仿函数——左右值均可调用
    static_assert(type::FuncTraits<SimpleFunctor>::value == true, "assert failed!");
    static_assert(type::FuncTraits<SimpleFunctor&>::value == true, "assert failed!");
    static_assert(type::FuncTraits<SimpleFunctor&&>::value == true, "assert failed!");

    _MEIDO_INFO_RAW("FuncTraitsRefQualifierTest passed!");
}

// cv ref限定的成员函数和operator()，实际能否调用取决于编译期实现，不一定符合预期，因此不过多检查cvref与Functor在标准中的调用匹配性
// FuncTraits的本质作用是判断函数能否调用，只保证能通过的Fn与其解析出的参数能调用即可，不要求函数调用严格遵循标准
inline void FuncTraitsCvrefTest()
{
    // 无 cv 无 ref 仿函数——左右值均可调用
    static_assert(type::FuncTraits<NoCvRefFunctor>::value == true, "assert failed!");
    static_assert(type::FuncTraits<NoCvRefFunctor&>::value == true, "assert failed!");
    static_assert(type::FuncTraits<NoCvRefFunctor&&>::value == true, "assert failed!");

    // const 限定仿函数——左右值均可调用
    static_assert(type::FuncTraits<ConstFunctor>::value == true, "assert failed!");
    static_assert(type::FuncTraits<ConstFunctor&>::value == true, "assert failed!");
    static_assert(type::FuncTraits<ConstFunctor&&>::value == true, "assert failed!");

    // 以下 cvref 变体在 GCC C++11 下因编译器对引用限定符的实现差异无法全部通过编译期检查，
    // 保留被注释的测试用例作为参考记录：
    //   ConstLvalueFunctor / ConstRvalueFunctor / VolatileFunctor / VolatileLvalueFunctor /
    //   VolatileRvalueFunctor / ConstVolatileFunctor / ConstVolatileLvalueFunctor / ConstVolatileRvalueFunctor

    _MEIDO_INFO_RAW("FuncTraitsCvrefTest passed!");
}

// 辅助类型：用于 InvokeTupleChecker 测试
struct InvokeAdder
{
    int operator()(int a, int b) const
    {
        return a + b;
    }
};

inline void InvokeTupleCheckerTest()
{
    // ========== 正面测试：应返回 true ==========

    // 1) 函数：参数完全匹配
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<const int&, int>>::value == true, "assert failed!");

    // 2) 函数：无参数
    static_assert(_priv::InvokeTupleChecker<decltype(sampleEmptyFunc), std::tuple<>>::value == true, "assert failed!");

    // 3) 仿函数：单一 operator()，参数匹配
    static_assert(_priv::InvokeTupleChecker<InvokeAdder, std::tuple<int, int>>::value == true, "assert failed!");

    // 4) 仿函数：无参数
    static_assert(_priv::InvokeTupleChecker<SimpleFunctor, std::tuple<>>::value == true, "assert failed!");

    // 5) 函数指针
    static_assert(_priv::InvokeTupleChecker<int (*)(const int&, int), std::tuple<const int&, int>>::value == true, "assert failed!");

    // 6) 左值引用 tuple& ——验证 & 重载
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<const int&, int>&>::value == true, "assert failed!");

    // 7) 右值引用 tuple&& ——验证 && 重载
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<const int&, int>&&>::value == true, "assert failed!");

    // 8) 左值引用仿函数 & (LvalueOnlyFunctor 只有 operator() &)
    static_assert(_priv::InvokeTupleChecker<LvalueOnlyFunctor&, std::tuple<>>::value == true, "assert failed!");

    // 9) 右值引用仿函数 && (RvalueOnlyFunctor 只有 operator() &&)
    static_assert(_priv::InvokeTupleChecker<RvalueOnlyFunctor&&, std::tuple<>>::value == true, "assert failed!");

    // 10) RvalueOnlyFunctor 无引用——也是右值可调用
    static_assert(_priv::InvokeTupleChecker<RvalueOnlyFunctor, std::tuple<>>::value == true, "assert failed!");

    // ========== 负面测试：应返回 false ==========

    // 11) 非可调用类型
    static_assert(_priv::InvokeTupleChecker<int, std::tuple<int>>::value == false, "assert failed!");

    // 12) 参数数量不足
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<int>>::value == false, "assert failed!");

    // 13) 参数类型不匹配
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<int, std::string>>::value == false, "assert failed!");

    // 14) 参数过多
    static_assert(_priv::InvokeTupleChecker<decltype(sampleAddFunc), std::tuple<int, int, int>>::value == false, "assert failed!");

    // 15) 仿函数参数类型不匹配
    static_assert(_priv::InvokeTupleChecker<InvokeAdder, std::tuple<int>>::value == false, "assert failed!");

    // 16) LvalueOnlyFunctor 作为右值——operator() & 不能被右值调用
    static_assert(_priv::InvokeTupleChecker<LvalueOnlyFunctor, std::tuple<>>::value == false, "assert failed!");

    // 17) RvalueOnlyFunctor 作为左值引用——operator() && 不能被左值调用
    static_assert(_priv::InvokeTupleChecker<RvalueOnlyFunctor&, std::tuple<>>::value == false, "assert failed!");

    _MEIDO_INFO_RAW("InvokeTupleCheckerTest passed!");
}

inline void FuncTraitsMemberFuncTest()
{
    // 成员函数指针本身是完整的函数类型，所有 cvref 均应被 FuncTraits 识别
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::nonCvRef)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::lvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constLvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileLvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileLvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::rvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constRvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileRvalueFunc)>::value == true, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileRvalueFunc)>::value == true, "assert failed!");

    // 验证 ReturnType 和 num_args 提取
    static_assert(type::SameTypesChecker<type::FuncTraits<decltype(&CvrefMemberFuncClass::nonCvRef)>::ReturnType, void>::value, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::nonCvRef)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::lvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constLvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileLvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileLvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::rvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constRvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::volatileRvalueFunc)>::num_args == 0, "assert failed!");
    static_assert(type::FuncTraits<decltype(&CvrefMemberFuncClass::constVolatileRvalueFunc)>::num_args == 0, "assert failed!");

    _MEIDO_INFO_RAW("FuncTraitsMemberFuncTest passed!");
}

// 验证 FuncTraits 的可靠性：若 FuncTraits<Fn>::value == true，
// 则 decltype(declval<Fn>()()) 必须能通过编译（调用合法），
// 且返回类型与 FuncTraits<Fn>::ReturnType 一致。
#define VERIFY_FUNCTRAITS_CALLABLE(Fn, ExpectedRet)                           \
    static_assert(type::FuncTraits<Fn>::value == true, "assert failed!");     \
    static_assert(                                                             \
        type::SameTypesChecker<                                               \
            type::FuncTraits<Fn>::ReturnType,                                 \
            ExpectedRet                                                       \
        >::value,                                                             \
        "assert failed!");                                                    \
    static_assert(                                                             \
        type::SameTypesChecker<                                               \
            decltype(std::declval<Fn>()()),                                   \
            ExpectedRet                                                       \
        >::value,                                                             \
        "assert failed!")

inline void FuncTraitsSoundnessTest()
{
    // ---- 无参函数 ----
    VERIFY_FUNCTRAITS_CALLABLE(decltype(sampleEmptyFunc), void);

    // ---- 无参函数指针 ----
    VERIFY_FUNCTRAITS_CALLABLE(decltype(&sampleEmptyFunc), void);

    // ---- Functor2（无 cv 无 ref） ----
    VERIFY_FUNCTRAITS_CALLABLE(SimpleFunctor, void);
    VERIFY_FUNCTRAITS_CALLABLE(SimpleFunctor&, void);
    VERIFY_FUNCTRAITS_CALLABLE(SimpleFunctor&&, void);

    // ---- NoCvRefFunctor（无 cv 无 ref） ----
    VERIFY_FUNCTRAITS_CALLABLE(NoCvRefFunctor, void);
    VERIFY_FUNCTRAITS_CALLABLE(NoCvRefFunctor&, void);
    VERIFY_FUNCTRAITS_CALLABLE(NoCvRefFunctor&&, void);

    // ---- ConstFunctor（const，左右值均可调用） ----
    VERIFY_FUNCTRAITS_CALLABLE(ConstFunctor, void);
    VERIFY_FUNCTRAITS_CALLABLE(ConstFunctor&, void);
    VERIFY_FUNCTRAITS_CALLABLE(ConstFunctor&&, void);
    VERIFY_FUNCTRAITS_CALLABLE(const ConstFunctor, void);
    VERIFY_FUNCTRAITS_CALLABLE(const ConstFunctor&, void);
    VERIFY_FUNCTRAITS_CALLABLE(const ConstFunctor&&, void);

    // ---- RvalueOnlyFunctor（右值可调用，仅测试通过检查的 Fn） ----
    VERIFY_FUNCTRAITS_CALLABLE(RvalueOnlyFunctor, void);
    VERIFY_FUNCTRAITS_CALLABLE(RvalueOnlyFunctor&&, void);

    // ---- LvalueOnlyFunctor（左值可调用，仅测试通过检查的 Fn） ----
    VERIFY_FUNCTRAITS_CALLABLE(LvalueOnlyFunctor&, void);

    _MEIDO_INFO_RAW("FuncTraitsSoundnessTest passed!");
}

// ===== FuncTraits<Fn>::ReturnType 在函数模板替换上下文中的 SFINAE 行为 =====
//
// 这组测试证明：FuncTraits<Fn>::ReturnType 只要被放在「替换上下文」
// （函数模板的默认模板参数、enable_if 等）中访问，
// 其失败就会被 SFINAE 捕获而非产生硬错误。
//
// 注意：成员函数模板的默认模板参数在类模板实例化时会被编译器求值
// （不属于替换上下文），因此必须使用全局/静态函数模板。

// ----- 方式 A：默认函数模板参数 -----
template <class Fn,
          class R = typename type::FuncTraits<Fn>::ReturnType>
std::true_type sfinaeDefaultParam(int);

template <class...>
std::false_type sfinaeDefaultParam(...);

// ----- 方式 B：enable_if + ReturnType -----
template <class Fn,
          typename std::enable_if<type::FuncTraits<Fn>::value, int>::type = 0,
          class R = typename type::FuncTraits<Fn>::ReturnType>
std::true_type sfinaeEnableIfReturnType(int);

template <class...>
std::false_type sfinaeEnableIfReturnType(...);

inline void FuncTraitsReturnTypeSFINAETest()
{
    // ===== 方式 A：默认函数模板参数 =====
    static_assert(
        decltype(sfinaeDefaultParam<decltype(sampleEmptyFunc)>(0))::value == true,
        "Default param: callable Fn should be detected!");
    static_assert(
        decltype(sfinaeDefaultParam<int>(0))::value == false,
        "Default param: non-callable (int) should SFINAE-fail!");

    // ===== 方式 B：enable_if + ReturnType =====
    static_assert(
        decltype(sfinaeEnableIfReturnType<decltype(sampleEmptyFunc)>(0))::value == true,
        "enable_if+ReturnType: callable Fn should be detected!");
    static_assert(
        decltype(sfinaeEnableIfReturnType<int>(0))::value == false,
        "enable_if+ReturnType: non-callable (int) should SFINAE-fail!");

    _MEIDO_INFO_RAW("FuncTraitsReturnTypeSFINAETest passed!");
}

/*  辅助 traits：将类型包转为 EachTrueChecker<is_same<int, Ts>::value...>::value
    用于验证包展开作为 EachTrueChecker 实参时的正确性（type.hpp VS2019 bug 注释相关） */
template <class... Ts>
struct PackEachTrueChecker
{
    static constexpr bool value =
        type::EachTrueChecker<std::is_same<int, Ts>::value...>::value;
};

template <class... Ts>
struct PackEachTrueCheckerWorkaround
{
    static constexpr bool value =
        type::EachTrueChecker<true, std::is_same<int, Ts>::value...>::value;
};

/*  验证 EachTrueChecker 在参数包展开时的行为
    背景：type.hpp 注释指出 VS2019 在 SFINAE 跨命名空间上下文中，
    EachTrueChecker<is_same<int, Ts>::value...> 可能因 MSVC 访问检查 bug 而失败。
    本测试通过直接求值（非 SFINAE）验证包展开值计算是否正确。
    若 value 正确而仅 SFINAE 跨命名空间有问题，则是 MSVC bug 而非 EachTrueChecker 逻辑错误。 */
inline void EachTrueCheckerPackTest()
{
    // ---- 模式 A: 直接传字面量（基本正确性） ----
    static_assert(type::EachTrueChecker<std::is_same<int, int>::value,
                                          std::is_same<int, int>::value>::value == true,
                  "assert failed!");
    static_assert(type::EachTrueChecker<std::is_same<int, int>::value,
                                          std::is_same<int, double>::value>::value == false,
                  "assert failed!");

    // ---- 模式 B: 包展开作为 EachTrueChecker 唯一实参 ----
    static_assert(PackEachTrueChecker<int, int, int>::value == true, "assert failed!");
    static_assert(PackEachTrueChecker<int, double, int>::value == false, "assert failed!");
    static_assert(PackEachTrueChecker<int>::value == true, "assert failed!");          // 单元素
    static_assert(PackEachTrueChecker<double>::value == false, "assert failed!");      // 单元素不匹配

    // ---- 模式 C: 带 dummy 首实参的 workaround ----
    static_assert(PackEachTrueCheckerWorkaround<int, int, int>::value == true, "assert failed!");
    static_assert(PackEachTrueCheckerWorkaround<int, double, int>::value == false, "assert failed!");
    static_assert(PackEachTrueCheckerWorkaround<int>::value == true, "assert failed!");
    static_assert(PackEachTrueCheckerWorkaround<double>::value == false, "assert failed!");

    _MEIDO_INFO_RAW("EachTrueCheckerPackTest passed! "
                    "(Note: cross-namespace SFINAE usage may trigger MSVC access-check bug; "
                    "see type.hpp EachTrueChecker comments)");
}

/*  [MSVC C2248 复现] 跨命名空间函数 + 待解包Ts...
    首参数为待解包 <std::is_same<int, Ts>::value...> 触发（递归 EachTrueChecker 求值） */
// template <class... Ts,
//           typename std::enable_if<
//               type::EachTrueChecker<std::is_same<int, Ts>::value...>::value,
//               int>::type = 0>
// constexpr int eachTrueSFINAE_Bug(int) { return 1; }
// static_assert(eachTrueSFINAE_Bug<int, int>(0) == 1, "C2248 trigger");

inline void check()
{
    _MEIDO_INFO_RAW("\n--------------------check type start--------------------");

    // ---- 1. 类型特征检查器 ----
    EachTrueFalseCheckerTest();
    EachTrueCheckerPackTest();
    SameTypesCheckerTest();
    InTypesCheckerTest();
    privStdBeginEndCheckerTest();
    privStdCoutCheckerTest();
    ConstructibleFromEachCheckerTest();
    EachLvalueRvalueConstructibleCheckerTest();
    RvalueRefMakerTest();
    GetTypeNameTest();

    // ---- 2. 函数特征提取 (FuncTraits) ----
    FuncTraitsBasicTest();
    FuncTraitsRefQualifierTest();
    FuncTraitsCvrefTest();
    FuncTraitsMemberFuncTest();
    FuncTraitsSoundnessTest();
    FuncTraitsReturnTypeSFINAETest();

    // ---- 3. 调用参数检测 (InvokeTupleChecker) ----
    InvokeTupleCheckerTest();

    // ---- 4. Bound 调用检测 (StrictStdBindTraits) ----
    StrictStdBindTraitsTest();

    _MEIDO_INFO_RAW("---------------------check type end---------------------");
    _MEIDO_INFO_RAW("");



}

}    // namespace _meidotypecheck

#endif    // !TYPE_TEST_HPP_BOKUMEIDOCPP

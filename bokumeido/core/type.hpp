/*  bokumeidocpp库的类型相关工具 */
#pragma once
#ifndef TYPE_HPP_BOKUMEIDOCPP
#define TYPE_HPP_BOKUMEIDOCPP

#include <functional>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <vector>

#if defined(__GLIBCXX__) || defined(_LIBCPP_ABI_VERSION)
#include <cxxabi.h>
#endif

#include "base.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace type
{
    // 获取类型的名字字符串
    template <class T>
    inline const std::string& getTypeName();

    /*  将T类型转换为自身的右值引用类型，用于区别于std::add_rvalue_reference
        用法：RvalueRefMaker<T>::Type  */
    template <class T>
    struct RvalueRefMaker;

    /*  检查T是否为存在const、volatile和引用符号中的任意一个作为顶层修饰
        用法：AnyTopCVRefChecker<T>::value, 类型为constexpr bool   */
    template <class T>
    struct AnyTopCVRefChecker;

    /*  判断是否每个给出的编译期布尔量都为true
        存在问题：
        - 在MSVC中测试，无法正确处理模板template<class... Ts, typename std::enable_if<type::EachTrueChecker<std::is_same<int, Ts>::value...>::value, int>::type = 0>，会触发error C2248
        - 可能是MSVC的实现bug，需要为改为template<class... Ts, typename std::enable_if<type::EachTrueChecker<true, std::is_same<int, Ts>::value...>::value, int>::type = 0>，不能用待解包类型作为首参数
        - 其他类似的模板在MSVC中可能会存在类似的问题
        用法：EachTrueChecker<values...>::value, 类型为constexpr bool  */
    template <bool bvalue, bool... bvalues>
    struct EachTrueChecker;

    /*  判断是否每个给出的编译期布尔量都为false
        用法：EachFalseChecker<values...>::value, 类型为constexpr bool  */
    template <bool bvalue, bool... bvalues>
    struct EachFalseChecker;


    /*  检查Type0、Type1、Types...是否为相同类型，不忽略const、引用等符号
        用法：SameTypesChecker<Type0, Type1, Types...>::value, 类型为constexpr bool   */
    template <class Type0, class Type1, class... Types>
    struct SameTypesChecker;

    /*  检查T是否为Type、Types...中的一个，不忽略const、引用等符号
        用法：InTypesChecker<T, Type, Types...>::value, 类型为constexpr bool   */
    template <class T, class Type, class... Types>
    struct InTypesChecker;

    /*  检查T、Ts中的每一个类型是否都支持std::cout<<，支持模板SFINAE特性
        - 每一个T、Ts类型的对象都重载了std::ostream& operator<<(std::ostream&, const T&)
        - 对每一个T、Ts类型查找operator<<的范围为std命名空间、类型T所在命名空间及其关联命名空间，以及全局命名空间
        用法：StdCoutEachChecker<T, Ts...>::value, 类型为constexpr bool   */
    template <class T, class... Ts>
    struct StdCoutEachChecker;

    /*  检查DstT是否可以由T、Ts...类型对象中的每一个单独构造或引用绑定，支持模板SFINAE特性
        - 检查规则参照std::is_constructible<DstT, T>
        - T、Ts...中的非引用类型，会被作为右值类型进行检查
        - DstT作为非引用类型时，检查DstT是否可以由T、Ts...中每一个的对象构造
        - DstT作为引用类型时，检查DstT是否可以绑定到每一个T、Ts...对象上
        - 构造过程中的隐式转换也是被支持的
        用法：ConstructibleFromEachChecker<DstT, T, Ts...>::value，类型为constexpr bool   */
    template <class DstT, class T, class... Ts>
    struct ConstructibleFromEachChecker;

    /*  检查T、Ts...类型中的每一个是否都支持使用自身的左值进行构造或引用绑定，支持模板SFINAE特性
        - 对于T、Ts...中的非引用类型，检查其能否由自身的左值对象构造
        - 对于T、Ts...中的引用类型，检查其能否绑定到自身的左值对象上
        - 与std::is_copy_constructible的区别在于对引用类型的处理也有意义
        用法：EachLvalueConstructibleChecker<T, Ts...>::value，类型为constexpr bool   */
    template <class T, class... Ts>
    struct EachLvalueConstructibleChecker;

    /*  检查T、Ts...类型中的每一个是否都支持使用自身的右值进行构造或引用绑定，支持模板SFINAE特性
        - 对于T、Ts...中的非引用类型，检查其能否由自身的右值对象构造
        - 对于T、Ts...中的引用类型，检查其能否绑定到自身的右值对象上
        - 与std::is_move_constructible的区别在于对引用类型的处理也有意义
        用法：EachRvalueConstructibleChecker<T, Ts...>::value，类型为constexpr bool   */
    template <class T, class... Ts>
    struct EachRvalueConstructibleChecker;

    /*  对Fn进行检查，判断Fn是否为可解析的函数、仿函数或成员函数，并获取返回值、参数数量等信息，支持模板SFINAE特性
        - Fn支持函数、函数指针、成员函数指针和仿函数类型
        - 传入Fn的c、v、ref限定会影响其匹配能否通过
        - Fn接收有多个重载的函数和成员函数时，需要显式指定函数类型，如FuncTraits<decltype((void(*)(int&))func1)>
        - Fn是仿函数时，如果仿函数重载了多个operator()，那么FuncTraits<Fn>::value为false
        - 由于编译器的设定，对于非引用的基本类型，ReturnType不保留顶层cv修饰（如返回const int会被忽略为int）
        用法：
        - FuncTraits<Fn>::value，判断Fn是否为有效函数，类型为constexpr bool
        - FuncTraits<Fn>::ReturnType，获取Fn的返回值类型，value检查为false时ReturnType不存在
        - FuncTraits<Fn>::ArgsTupleType，获取Fn的参数列表类型组成的std::tuple类型，value检查为false时ArgsTupleType不存在
        - FuncTraits<Fn>::num_args，类型为constexpr size_t，获得Fn的参数数量，value检查为false时num_args不存在   */
    template <class Fn>
    struct FuncTraits;

    /*  检查Fn和Args...参数是否能使用std::bind绑定，且具有更严格的限制，支持模板SFINAE特性
        - Fn支持函数、函数指针、成员函数指针、仿函数类型以及它们的reference_warpper包装
        - Fn接收重载函数时，需要显式指定函数类型，如StdBindTraits<decltype((void(*)(int&))func1), int&>
        - Fn是仿函数时，自身的去引用类型必须支持使用自身的左值和右值对象进行构造，且不是volatile类型
        - Fn是具有多个operator()重载的仿函数时，参数类型与Fn的一个operator()匹配即可
        - const、volatile限定的成员函数只能由const、volatile对象调用，对于仿函数也是一样
        - 要求Args...中非C数组类型的去引用类型均支持使用自身的左值和右值对象进行构造
        - 要求Fn的参数类型不能为右值引用
        - 由于编译器的设定，对于非引用的基本类型，ReturnType不保留顶层cv修饰（如返回const int会被忽略为int）
        - 其他要求参考std::bind规则
        用法：
        - StrictStdBindTraits<Fn>::value，判断Fn是否为有效函数类型，类型为constexpr bool
        - StrictStdBindTraits<Fn>::ReturnType，获取Fn的返回值类型，Fn和Args...不匹配时ReturnType不存在  */
    template <class Fn, class... Args>
    struct StrictStdBindTraits;

}    // namespace type










/*--------------------------------------------内部实现--------------------------------------------*/

namespace _priv
{
    /*  检查T是否拥有类似于标准的STL容器的begin()和end()接口，支持模板SFINAE特性
        - T类型具有begin()和end()接口
        - T类型的begin()和end()接口返回的迭代器类型为std::remove_reference<U>::type::iterator或std::remove_reference<U>::type::const_iterator
        - T类型的begin()返回的迭代器可用被*解引用
        - T类型的begin()返回的迭代器支持++操作符，且前缀的++操作符返回迭代器的引用，后缀的++操作符返回迭代器的拷贝
        用法：StdBeginEndChecker<T>::value, 类型为constexpr bool   */
    template <class T>
    struct StdBeginEndChecker
    {
    private:
        // 之所以用U而不是T，是因为为T的话，模板类型就被类的模板类型确定了，函数调用不再具有SFINAE特性
        template <class U, typename std::enable_if<!std::is_const<U>::value && std::is_same<decltype(std::declval<U>().begin()), typename std::remove_reference<U>::type::iterator>::value && std::is_same<decltype(std::declval<U>().end()), typename std::remove_reference<U>::type::iterator>::value, int>::type = 0>
        static std::true_type checkExist(int);

        template <class U, typename std::enable_if<std::is_const<U>::value && std::is_same<decltype(std::declval<U>().begin()), typename std::remove_reference<U>::type::const_iterator>::value && std::is_same<decltype(std::declval<U>().end()), typename std::remove_reference<U>::type::const_iterator>::value, int>::type = 0>
        static std::true_type checkExist(int);

        template <class...>
        static std::false_type checkExist(...);

        template <class U, typename = decltype(*(std::declval<typename std::remove_reference<U>::type::iterator>()))>
        static std::true_type checkDeref(int);

        template <class...>
        static std::false_type checkDeref(...);

        template <class U, typename std::enable_if<!std::is_const<U>::value && std::is_same<decltype(++std::declval<U>().begin()), typename std::remove_reference<U>::type::iterator&>::value && std::is_same<decltype(std::declval<U>().begin()++), typename std::remove_reference<U>::type::iterator>::value, int>::type = 0>
        static std::true_type checkIncrementOperator(int);

        template <class U, typename std::enable_if<std::is_const<U>::value && std::is_same<decltype(++std::declval<U>().begin()), typename std::remove_reference<U>::type::const_iterator&>::value && std::is_same<decltype(std::declval<U>().begin()++), typename std::remove_reference<U>::type::const_iterator>::value, int>::type = 0>
        static std::true_type checkIncrementOperator(int);

        template <class...>
        static std::false_type checkIncrementOperator(...);

        StdBeginEndChecker() = delete;

    public:
        static constexpr bool value = decltype(_priv::StdBeginEndChecker<T>::template checkExist<T>(0))::value && decltype(_priv::StdBeginEndChecker<T>::template checkDeref<T>(0))::value && decltype(_priv::StdBeginEndChecker<T>::template checkIncrementOperator<T>(0))::value;
    };
    template <class T>
    constexpr bool _priv::StdBeginEndChecker<T>::value;

    template <typename... T>
    struct _make_void
    {
        using type = void;
    };

    template <typename... T>
    using _void_t = typename _make_void<T...>::type;


    /*  检查T是否支持std::cout <<，支持模板SFINAE特性
        - T类型正确重载了operator<<(std::ostream&, const T&)
        - 对于T类型的对象obj，std::cout << obj的返回值类型为std::ostream&
        - 对T类型查找operator<<的范围为std命名空间、T所在命名空间及其关联命名空间，以及全局命名空间
        用法：StdCoutChecker<T>::value, 类型为constexpr bool   */
    template <class T>
    struct StdCoutChecker
    {
    private:
        // 之所以用U而不是T，是因为为T的话，模板类型就被类的模板类型确定了，函数调用不再具有SFINAE特性
        template <class U, typename std::enable_if<std::is_same<decltype(std::cout << std::declval<U>()), std::ostream&>::value, int>::type = 0>
        static std::true_type check(int);

        template <typename...>
        static std::false_type check(...);

        StdCoutChecker() = delete;

    public:
        static constexpr bool value = decltype(_priv::StdCoutChecker<T>::template check<T>(0))::value;
    };
    template <class T>
    constexpr bool _priv::StdCoutChecker<T>::value;


    template <class Fn, class Tuple>
    struct InvokeTupleChecker
    {
    private:
        template <class... Args>
        static auto check(std::tuple<Args...>&) -> decltype(std::declval<Fn>()(std::declval<Args>()...), std::true_type());

        template <class... Args>
        static auto check(std::tuple<Args...>&&) -> decltype(std::declval<Fn>()(std::declval<Args>()...), std::true_type());

        template <class...>
        static std::false_type check(...);

        InvokeTupleChecker() = delete;

    public:
        template <class... Args>
        static auto checkRet(std::tuple<Args...>&) -> decltype(std::declval<Fn>()(std::declval<Args>()...));

        template <class... Args>
        static auto checkRet(std::tuple<Args...>&&) -> decltype(std::declval<Fn>()(std::declval<Args>()...));

        static constexpr bool value = decltype(_priv::InvokeTupleChecker<Fn, Tuple>::template check(std::declval<Tuple>()))::value;
    };
    template <class Fn, class Tuple>
    constexpr bool _priv::InvokeTupleChecker<Fn, Tuple>::value;


    struct FunctionCheckerBase
    {
    public:
        template <class Ret, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (*)(Arguments...));

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...));

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) volatile);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const volatile);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) &);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) volatile&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const volatile&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) &&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const&&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) volatile&&);

        template <class Ret, class ObjType, class... Arguments>
        static std::tuple<Arguments...> checkArguments(Ret (ObjType::*)(Arguments...) const volatile&&);

        template <class Functor, class ArgumentsTuple = decltype(checkArguments(&std::remove_reference<Functor>::type::operator()))>    // 通过Functor找operator()时必须为非引用类型
        static ArgumentsTuple checkArguments(Functor&&);

        template <class Ret, class... Arguments>
        static Ret checkRet(Ret (*)(Arguments...));

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...));

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) volatile);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const volatile);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) &);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) volatile&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const volatile&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) &&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const&&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) volatile&&);

        template <class Ret, class ObjType, class... Arguments>
        static Ret checkRet(Ret (ObjType::*)(Arguments...) const volatile&&);


        template <class Functor, class ArgsTuple = decltype(_priv::FunctionCheckerBase::checkArguments(std::declval<Functor>())), class Ret = decltype(_priv::InvokeTupleChecker<Functor, ArgsTuple>::checkRet(std::declval<ArgsTuple>()))>
        static Ret checkRet(Functor&&);

        template <class Fn, class Ret = decltype(checkRet(std::declval<Fn>()))>
        static std::true_type checkValidity(int);

        template <class...>
        static std::false_type checkValidity(...);
    };

    template <bool Checker, class Fn>
    struct FunctionCheckerHelper
    {
    public:
        static constexpr bool value = false;
    };
    template <bool Checker, class Fn>
    constexpr bool _priv::FunctionCheckerHelper<Checker, Fn>::value;

    template <class Fn>
    struct FunctionCheckerHelper<true, Fn>
    {
    public:
        static constexpr bool value = true;
        using ReturnType = decltype(_priv::FunctionCheckerBase::checkRet(std::declval<Fn>()));
        using ArgsTupleType = decltype(_priv::FunctionCheckerBase::checkArguments(std::declval<Fn>()));
        static constexpr size_t num_args = std::tuple_size<ArgsTupleType>::value;
    };
    template <class Fn>
    constexpr bool _priv::FunctionCheckerHelper<true, Fn>::value;
    template <class Fn>
    constexpr size_t _priv::FunctionCheckerHelper<true, Fn>::num_args;


    // std::bind在qnx的gcc4.7.3上，无法绑定const仿函数和它的非const operator()，在其他编译器上却可以，因此统一按照严格的限定处理
    template <class Fn, class... Args>
    struct FunctorBindChecker
    {
        template <class Func, class... Arguments, typename std::enable_if<std::is_class<typename std::remove_reference<Func>::type>::value, int>::type = 0, class Ret = decltype(std::declval<Func>()(std::declval<typename std::add_lvalue_reference<Arguments>::type>()...))>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        template <class Func>
        static Func checkFn(std::reference_wrapper<Func>);

        template <class Func>
        static Func checkFn(Func&&);

        // 在当前的使用情况中，Func不可能作为void被传进来
        using Func = decltype(_priv::FunctorBindChecker<Fn, Args...>::template checkFn(std::declval<Fn>()));

        static constexpr bool value = decltype(_priv::FunctorBindChecker<Func, Args...>::template check<Func, Args...>(0))::value;
    };
    template <typename Fn, typename... Args>
    constexpr bool _priv::FunctorBindChecker<Fn, Args...>::value;

    // 会去除Args...的引用后再检查
    template <class... Args>
    struct EachConstructibleByLRvalueChecker
    {
    private:
        template <class Argument, class... Arguments, typename std::enable_if<type::EachTrueChecker<std::is_array<typename std::remove_reference<Argument>::type>::value, _priv::EachConstructibleByLRvalueChecker<Arguments>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class Argument, class... Arguments, typename std::enable_if<!std::is_array<typename std::remove_reference<Argument>::type>::value, int>::type = 0, typename std::enable_if<type::EachTrueChecker<type::ConstructibleFromEachChecker<typename std::remove_reference<Argument>::type, typename std::add_lvalue_reference<Argument>::type, typename type::RvalueRefMaker<Argument>::Type>::value, _priv::EachConstructibleByLRvalueChecker<Arguments>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class... Arguments, typename std::enable_if<(sizeof...(Arguments) == 0), int>::type = 0>
        static std::true_type check(int);

        template <class Argument, class... Arguments>
        static std::false_type check(...);

    public:
        static constexpr bool value = decltype(_priv::EachConstructibleByLRvalueChecker<Args...>::template check<Args...>(0))::value;
    };
    template <class... Args>
    constexpr bool _priv::EachConstructibleByLRvalueChecker<Args...>::value;

    template <class Fn>
    struct MemFuncCvChecker
    {
    public:
        template <class Ret, class ObjType, class... Arguments>
        static std::true_type checkOnlyConst(Ret (ObjType::*)(Arguments...) const);

        template <class...>
        static std::false_type checkOnlyConst(...);

        template <class Ret, class ObjType, class... Arguments>
        static std::true_type checkOnlyVolatile(Ret (ObjType::*)(Arguments...) volatile);

        template <class...>
        static std::false_type checkOnlyVolatile(...);

        template <class Ret, class ObjType, class... Arguments>
        static std::true_type checkConstVolatile(Ret (ObjType::*)(Arguments...) const volatile);

        template <class...>
        static std::false_type checkConstVolatile(...);

        template <class Ret, class ObjType, class... Arguments>
        static ObjType checkObjType(Ret (ObjType::*)(Arguments...));

        template <class Ret, class ObjType, class... Arguments>
        static ObjType checkObjType(Ret (ObjType::*)(Arguments...) const);

        template <class Ret, class ObjType, class... Arguments>
        static ObjType checkObjType(Ret (ObjType::*)(Arguments...) volatile);

        template <class Ret, class ObjType, class... Arguments>
        static ObjType checkObjType(Ret (ObjType::*)(Arguments...) const volatile);

    public:
        // 目前在使用时Fn不可能为void
        static constexpr bool is_only_const = decltype(_priv::MemFuncCvChecker<Fn>::template checkOnlyConst(std::declval<Fn>()))::value;
        static constexpr bool is_only_volatile = decltype(_priv::MemFuncCvChecker<Fn>::template checkOnlyVolatile(std::declval<Fn>()))::value;
        static constexpr bool is_const_volatile = decltype(_priv::MemFuncCvChecker<Fn>::template checkConstVolatile(std::declval<Fn>()))::value;

        static constexpr bool is_const = _priv::MemFuncCvChecker<Fn>::is_only_const || _priv::MemFuncCvChecker<Fn>::is_const_volatile;
        static constexpr bool is_volatile = _priv::MemFuncCvChecker<Fn>::is_only_volatile || _priv::MemFuncCvChecker<Fn>::is_const_volatile;
        static constexpr bool is_no_cv = !_priv::MemFuncCvChecker<Fn>::is_const && !_priv::MemFuncCvChecker<Fn>::is_volatile;
    };
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_only_const;
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_only_volatile;
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_const_volatile;
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_const;
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_volatile;
    template <class Fn>
    constexpr bool _priv::MemFuncCvChecker<Fn>::is_no_cv;

    // qnx的gcc4.7.3无法正确处理std::bind绑定非cv限定的成员函数和cv限定的对象，以及std::declval<ObjT>()->*std::declval<Func>()的情况，因此添加这个检查
    template <class MemFn, class ObjMaybePtr, class... Args>
    struct MemFnObjCvMatchChecker
    {
        template <class Obj>
        static Obj checkObj(std::reference_wrapper<Obj>);
        template <class Obj>
        static Obj checkObj(const Obj&);

        using ObjNoStdRef = decltype(_priv::MemFnObjCvMatchChecker<MemFn, ObjMaybePtr, Args...>::template checkObj(std::declval<ObjMaybePtr>()));
        using Obj = typename std::remove_reference<typename std::remove_pointer<ObjNoStdRef>::type>::type;
        using DecayObj = typename std::remove_cv<Obj>::type;
        // static constexpr bool only_const_match = _priv::MemFuncCvChecker<MemFn>::is_only_const && !std::is_volatile<Obj>::value;
        // static constexpr bool only_volatile_match = _priv::MemFuncCvChecker<MemFn>::is_only_volatile && !std::is_const<Obj>::value;
        // static constexpr bool const_volatile_match = _priv::MemFuncCvChecker<MemFn>::is_const_volatile;
        // static constexpr bool no_cv_match = _priv::MemFuncCvChecker<MemFn>::is_no_cv && !std::is_const<Obj>::value && !std::is_volatile<Obj>::value;

    public:
        // 在当前的使用情况中，MemFn不可能作为void被传进来
        // 每个&&子表达式显式加括号，避免 -Wlogical-op-parentheses（语义与&&优先于||时一致）
        static constexpr bool value = (_priv::MemFuncCvChecker<MemFn>::is_const_volatile || (_priv::MemFuncCvChecker<MemFn>::is_only_const && !std::is_volatile<Obj>::value) || (_priv::MemFuncCvChecker<MemFn>::is_only_volatile && !std::is_const<Obj>::value) || (_priv::MemFuncCvChecker<MemFn>::is_no_cv && !std::is_const<Obj>::value && !std::is_volatile<Obj>::value)) && (std::is_same<decltype(_priv::MemFuncCvChecker<MemFn>::template checkObjType(std::declval<MemFn>())), DecayObj>::value || std::is_base_of<decltype(_priv::MemFuncCvChecker<MemFn>::template checkObjType(std::declval<MemFn>())), DecayObj>::value);
    };
    template <class MemFn, class ObjMaybePtr, class... Args>
    constexpr bool _priv::MemFnObjCvMatchChecker<MemFn, ObjMaybePtr, Args...>::value;

    template <class Fn, class... Args>
    struct StdBindTraitsBase
    {
    public:
        template <class Func, class... Arguments, class DecayFunc = typename std::remove_reference<Func>::type, typename std::enable_if<!std::is_class<DecayFunc>::value && !std::is_member_function_pointer<DecayFunc>::value && (type::FuncTraits<Func>::num_args == sizeof...(Arguments)) && _priv::EachConstructibleByLRvalueChecker<Arguments...>::value, int>::type = 0>
        static std::true_type checkConditionsUnsupportSFINAE(int);

        // 检查Functor和std::ref包装的函数
        template <class Func, class... Arguments, class DecayFunc = typename std::remove_reference<Func>::type, typename std::enable_if<std::is_class<DecayFunc>::value && !std::is_volatile<DecayFunc>::value, int>::type = 0, typename std::enable_if<_priv::EachConstructibleByLRvalueChecker<Func, Arguments...>::value && _priv::FunctorBindChecker<Func, Arguments...>::value, int>::type = 0>
        static std::true_type checkConditionsUnsupportSFINAE(int);

        template <class Func, class... Arguments, typename std::enable_if<std::is_member_function_pointer<typename std::remove_reference<Func>::type>::value && (type::FuncTraits<Func>::num_args + 1 == sizeof...(Arguments)) && _priv::EachConstructibleByLRvalueChecker<Arguments...>::value, int>::type = 0, typename std::enable_if<_priv::MemFnObjCvMatchChecker<Func, Arguments...>::value, int>::type = 0>
        static std::true_type checkConditionsUnsupportSFINAE(int);

        template <class...>
        static std::false_type checkConditionsUnsupportSFINAE(...);

        template <class Func, class... Arguments, typename std::enable_if<decltype(_priv::StdBindTraitsBase<Func, Arguments...>::template checkConditionsUnsupportSFINAE<Func, Arguments...>(0))::value, int>::type = 0>
        static auto checkRet(int) -> decltype(std::bind(std::forward<Func>(std::declval<Func>()), std::forward<Arguments>(std::declval<Arguments>())...)());

        // template<class Func, class... Arguments, class DecayFunc = decltype(std::declval<Func>().get())>
        // static auto checkValue(int) -> decltype(_priv::StdBindTraitsBase<DecayFunc, Arguments...>::template checkRet<DecayFunc, Arguments...>(0), std::true_type());

        template <class Func, class... Arguments>
        static auto checkValue(int) -> decltype(_priv::StdBindTraitsBase<Func, Arguments...>::template checkRet<Func, Arguments...>(0), std::true_type());

        template <class...>
        static std::false_type checkValue(...);
    };

    template <bool Checker, class Fn, class... Args>
    struct StdBindTraitsHelper
    {
    public:
        static constexpr bool value = decltype(_priv::StdBindTraitsBase<Fn, Args...>::template checkValue<Fn, Args...>(0))::value;
    };
    template <bool Checker, class Fn, class... Args>
    constexpr bool _priv::StdBindTraitsHelper<Checker, Fn, Args...>::value;

    template <class Fn, class... Args>
    struct StdBindTraitsHelper<true, Fn, Args...>
    {
    public:
        static constexpr bool value = decltype(_priv::StdBindTraitsBase<Fn, Args...>::template checkValue<Fn, Args...>(0))::value;
        using ReturnType = decltype(_priv::StdBindTraitsBase<Fn, Args...>::template checkRet<Fn, Args...>(0));
    };
    template <class Fn, class... Args>
    constexpr bool _priv::StdBindTraitsHelper<true, Fn, Args...>::value;


    template <class T>
    struct AnyCVRefChecker
    {
    private:
        template <class U, typename std::enable_if<std::is_const<U>::value || std::is_volatile<U>::value || std::is_reference<U>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

    public:
        static constexpr bool value = decltype(_priv::AnyCVRefChecker<T>::template check<T>(0))::value;
    };
    template <class T>
    constexpr bool _priv::AnyCVRefChecker<T>::value;


    template <class T>
    struct CircularQueueElemChecker
    {
    private:
        template <class U, typename std::enable_if<!std::is_void<U>::value && !_priv::AnyCVRefChecker<U>::value && (std::is_assignable<U&, const U&>::value || std::is_assignable<U&, U&&>::value) && std::is_default_constructible<U>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

    public:
        static constexpr bool value = decltype(_priv::CircularQueueElemChecker<T>::template check<T>(0))::value;
    };
    template <class T>
    constexpr bool _priv::CircularQueueElemChecker<T>::value;


    template <class T>
    using NonBoolIntChecker = std::integral_constant<bool, std::is_integral<T>::value && !std::is_same<typename std::remove_cv<T>::type, bool>::value>;

}    // namespace _priv


namespace type
{
    template <class T>
    inline const std::string& getTypeName()
    {
        thread_local std::string type_name;
        if (!type_name.empty())
            return type_name;
#if defined(__GLIBCXX__) || defined(_LIBCPP_ABI_VERSION)
        int status = 0;
        char* buf = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
        if (status == 0 && buf)
        {
            type_name.assign(buf, strlen(buf));
            free(buf);
            return type_name;
        }

#endif
        type_name = typeid(T).name();
        return type_name;
    }

    /*  将T类型转换为自身的右值引用类型，用于区别于std::add_rvalue_reference
        用法：RvalueRefMaker<T>::Type  */
    template <class T>
    struct RvalueRefMaker
    {
    public:
        using Type = typename std::add_rvalue_reference<typename std::remove_reference<T>::type>::type;
    };

    /*  检查T是否为存在const、volatile和引用符号中的任意一个作为顶层修饰
        用法：AnyTopCVRefChecker<T>::value, 类型为constexpr bool   */
    template <class T>
    struct AnyTopCVRefChecker
    {
    public:
        static constexpr bool value = _priv::AnyCVRefChecker<T>::value;
    };
    template <class T>
    constexpr bool type::AnyTopCVRefChecker<T>::value;


    /*  判断是否每个给出的编译期布尔量都为true
        存在问题：
        - 在MSVC中测试，无法正确处理模板template<class... Ts, typename std::enable_if<type::EachTrueChecker<std::is_same<int, Ts>::value...>::value, int>::type = 0>，会触发error C2248
        - 可能是MSVC的实现bug，需要为改为template<class... Ts, typename std::enable_if<type::EachTrueChecker<true, std::is_same<int, Ts>::value...>::value, int>::type = 0>，不能用待解包类型作为首参数
        - 其他类似的模板在MSVC中可能会存在类似的问题
        用法：EachTrueChecker<values...>::value, 类型为constexpr bool  */
    template <bool bvalue, bool... bvalues>
    struct EachTrueChecker
    {
    private:
        // 似乎对于模板中都是​​非类型参数(值参数)​​的情况，在VS中判断结果不符合预期，因此只能通过尾随返回类型的方式解决
        template <bool v, bool... vs, typename = typename std::enable_if<v, int>::type>
        static auto check(int) -> typename std::enable_if<type::EachTrueChecker<vs...>::value, std::true_type>::type;

        template <bool v, bool... vs>
        static auto check(int) -> typename std::enable_if<v && (sizeof...(vs) == 0), std::true_type>::type;

        template <bool v, bool... vs>
        static auto check(int) -> typename std::enable_if<!v, std::false_type>::type;

        template <bool...>
        static std::false_type check(...);

        EachTrueChecker() = delete;

    public:
        static constexpr bool value = decltype(type::EachTrueChecker<bvalue, bvalues...>::template check<bvalue, bvalues...>(0))::value;
    };
    template <bool bvalue, bool... bvalues>
    constexpr bool type::EachTrueChecker<bvalue, bvalues...>::value;

    /*  判断是否每个给出的编译期布尔量都为false
        用法：EachFalseChecker<values...>::value, 类型为constexpr bool  */
    template <bool bvalue, bool... bvalues>
    struct EachFalseChecker
    {
    private:
        template <bool v, bool... vs, typename = typename std::enable_if<!v, int>::type>
        static auto check(int) -> typename std::enable_if<type::EachFalseChecker<vs...>::value, std::true_type>::type;

        template <bool v, bool... vs>
        static auto check(int) -> typename std::enable_if<!v && (sizeof...(vs) == 0), std::true_type>::type;

        template <bool v, bool... vs>
        static auto check(int) -> typename std::enable_if<v, std::false_type>::type;

        template <bool...>
        static std::false_type check(...);

        EachFalseChecker() = delete;

    public:
        static constexpr bool value = decltype(type::EachFalseChecker<bvalue, bvalues...>::template check<bvalue, bvalues...>(0))::value;
    };
    template <bool bvalue, bool... bvalues>
    constexpr bool type::EachFalseChecker<bvalue, bvalues...>::value;

    /*  检查Type0、Type1、Types...是否为相同类型，不忽略const、引用等符号
        用法：SameTypesChecker<Type0, Type1, Types...>::value, 类型为constexpr bool   */
    template <class Type0, class Type1, class... Types>
    struct SameTypesChecker
    {
    private:
        template <class U0, class U1, class... Us, typename std::enable_if<type::EachTrueChecker<std::is_same<U0, U1>::value, std::is_same<U0, Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        SameTypesChecker() = delete;

    public:
        static constexpr bool value = decltype(type::SameTypesChecker<Type0, Type1, Types...>::template check<Type0, Type1, Types...>(0))::value;
    };
    template <class Type0, class Type1, class... Types>
    constexpr bool type::SameTypesChecker<Type0, Type1, Types...>::value;

    /*  检查T是否为Type、Types...中的一个，不忽略const、引用等符号
        用法：InTypesChecker<T, Type, Types...>::value, 类型为constexpr bool   */
    template <class T, class Type, class... Types>
    struct InTypesChecker
    {
    private:
        template <class U0, class U1, class... Us, typename std::enable_if<!type::EachFalseChecker<std::is_same<U0, U1>::value, std::is_same<U0, Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        InTypesChecker() = delete;

    public:
        static constexpr bool value = decltype(type::InTypesChecker<T, Type, Types...>::template check<T, Type, Types...>(0))::value;
    };
    template <class T, class Type, class... Types>
    constexpr bool type::InTypesChecker<T, Type, Types...>::value;


    /*  检查T、Ts中的每一个类型是否都支持std::cout<<，支持模板SFINAE特性
        - 每一个T、Ts类型的对象都重载了std::ostream& operator<<(std::ostream&, const T&)
        - 对每一个T、Ts类型查找operator<<的范围为std命名空间、类型T所在命名空间及其关联命名空间，以及全局命名空间
        用法：StdCoutEachChecker<T, Ts...>::value, 类型为constexpr bool   */
    template <class T, class... Ts>
    struct StdCoutEachChecker
    {
    private:
        template <class U, class... Us, typename std::enable_if<type::EachTrueChecker<_priv::StdCoutChecker<U>::value, _priv::StdCoutChecker<Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        StdCoutEachChecker() = delete;

    public:
        static constexpr bool value = decltype(type::StdCoutEachChecker<T, Ts...>::template check<T, Ts...>(0))::value;
    };
    template <class T, class... Ts>
    constexpr bool type::StdCoutEachChecker<T, Ts...>::value;

    /*  检查DstT是否可以由T、Ts...类型对象中的每一个单独构造或引用绑定，支持模板SFINAE特性
        - 检查规则参照std::is_constructible<DstT, T>
        - T、Ts...中的非引用类型，会被作为右值类型进行检查
        - DstT作为非引用类型时，检查DstT是否可以由T、Ts...中每一个的对象构造
        - DstT作为引用类型时，检查DstT是否可以绑定到每一个T、Ts...对象上
        - 构造过程中的隐式转换也是被支持的
        用法：ConstructibleFromEachChecker<DstT, T, Ts...>::value，类型为constexpr bool   */
    template <class DstT, class T, class... Ts>
    struct ConstructibleFromEachChecker
    {
    private:
        template <class DstU, class U, class... Us, typename std::enable_if<type::EachTrueChecker<std::is_constructible<DstU, U>::value, std::is_constructible<DstU, Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        ConstructibleFromEachChecker() = delete;

    public:
        static constexpr bool value = decltype(type::ConstructibleFromEachChecker<DstT, T, Ts...>::template check<DstT, T, Ts...>(0))::value;
    };
    template <class DstT, class T, class... Ts>
    constexpr bool type::ConstructibleFromEachChecker<DstT, T, Ts...>::value;

    /*  检查T、Ts...类型中的每一个是否都支持使用自身的左值进行构造或引用绑定，支持模板SFINAE特性
        - 对于T、Ts...中的非引用类型，检查其能否由自身的左值对象构造
        - 对于T、Ts...中的引用类型，检查其能否绑定到自身的左值对象上
        - 与std::is_copy_constructible的区别在于对引用类型的处理也有意义
        用法：EachLvalueConstructibleChecker<T, Ts...>::value，类型为constexpr bool   */
    template <class T, class... Ts>
    struct EachLvalueConstructibleChecker
    {
    private:
        template <class U, class... Us, typename std::enable_if<type::EachTrueChecker<std::is_constructible<U, typename std::add_lvalue_reference<U>::type>::value, type::EachLvalueConstructibleChecker<Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        EachLvalueConstructibleChecker() = delete;

    public:
        static constexpr bool value = decltype(type::EachLvalueConstructibleChecker<T, Ts...>::template check<T, Ts...>(0))::value;
    };
    template <class T, class... Ts>
    constexpr bool type::EachLvalueConstructibleChecker<T, Ts...>::value;

    /*  检查T、Ts...类型中的每一个是否都支持使用自身的右值进行构造或引用绑定，支持模板SFINAE特性
        - 对于T、Ts...中的非引用类型，检查其能否由自身的右值对象构造
        - 对于T、Ts...中的引用类型，检查其能否绑定到自身的右值对象上
        - 与std::is_move_constructible的区别在于对引用类型的处理也有意义
        用法：EachRvalueConstructibleChecker<T, Ts...>::value，类型为constexpr bool   */
    template <class T, class... Ts>
    struct EachRvalueConstructibleChecker
    {
    private:
        template <class U, class... Us, typename std::enable_if<type::EachTrueChecker<std::is_constructible<U, typename type::RvalueRefMaker<U>::Type>::value, type::EachRvalueConstructibleChecker<Us>::value...>::value, int>::type = 0>
        static std::true_type check(int);

        template <class...>
        static std::false_type check(...);

        EachRvalueConstructibleChecker() = delete;

    public:
        static constexpr bool value = decltype(type::EachRvalueConstructibleChecker<T, Ts...>::template check<T, Ts...>(0))::value;
    };
    template <class T, class... Ts>
    constexpr bool type::EachRvalueConstructibleChecker<T, Ts...>::value;

    /*  对Fn进行检查，判断Fn是否为可解析的函数、仿函数或成员函数，并获取返回值、参数数量等信息，支持模板SFINAE特性
        - Fn支持函数、函数指针、成员函数指针和仿函数类型
        - 传入Fn的c、v、ref限定会影响其匹配能否通过
        - Fn接收有多个重载的函数和成员函数时，需要显式指定函数类型，如FuncTraits<decltype((void(*)(int&))func1)>
        - Fn是仿函数时，如果仿函数重载了多个operator()，那么FuncTraits<Fn>::value为false
        - 由于编译器的设定，对于非引用的基本类型，ReturnType不保留顶层cv修饰（如返回const int会被忽略为int）
        用法：
        - FuncTraits<Fn>::value，判断Fn是否为有效函数，类型为constexpr bool
        - FuncTraits<Fn>::ReturnType，获取Fn的返回值类型，value检查为false时ReturnType不存在
        - FuncTraits<Fn>::ArgsTupleType，获取Fn的参数列表类型组成的std::tuple类型，value检查为false时ArgsTupleType不存在
        - FuncTraits<Fn>::num_args，类型为constexpr size_t，获得Fn的参数数量，value检查为false时num_args不存在   */
    template <class Fn>
    struct FuncTraits : public _priv::FunctionCheckerHelper<decltype(_priv::FunctionCheckerBase::template checkValidity<Fn>(0))::value, Fn>
    {
    private:
        FuncTraits() = delete;
    };

    /*  检查Fn和Args...参数是否能使用std::bind绑定，且具有更严格的限制，支持模板SFINAE特性
        - Fn支持函数、函数指针、成员函数指针、仿函数类型以及它们的reference_warpper包装
        - Fn接收重载函数时，需要显式指定函数类型，如StdBindTraits<decltype((void(*)(int&))func1), int&>
        - Fn是仿函数时，自身的去引用类型必须支持使用自身的左值和右值对象进行构造，且不是volatile类型
        - Fn是具有多个operator()重载的仿函数时，参数类型与Fn的一个operator()匹配即可
        - const、volatile限定的成员函数只能由const、volatile对象调用，对于仿函数也是一样
        - 要求Args...中非C数组类型的去引用类型均支持使用自身的左值和右值对象进行构造
        - 要求Fn的参数类型不能为右值引用
        - 由于编译器的设定，对于非引用的基本类型，ReturnType不保留顶层cv修饰（如返回const int会被忽略为int）
        - 其他要求参考std::bind规则
        用法：
        - StrictStdBindTraits<Fn>::value，判断Fn是否为有效函数类型，类型为constexpr bool
        - StrictStdBindTraits<Fn>::ReturnType，获取Fn的返回值类型，Fn和Args...不匹配时ReturnType不存在  */
    template <class Fn, class... Args>
    struct StrictStdBindTraits : public _priv::StdBindTraitsHelper<decltype(_priv::StdBindTraitsBase<Fn, Args...>::template checkValue<Fn, Args...>(0))::value, Fn, Args...>
    {
    private:
        StrictStdBindTraits() = delete;
    };

}    // namespace type
}    // namespace meido

#endif    // !TYPE_HPP_BOKUMEIDOCPP
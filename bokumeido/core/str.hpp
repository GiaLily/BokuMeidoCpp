/*  bokumeidocpp库的std::string字符串相关工具 */
#pragma once
#ifndef STR_HPP_BOKUMEIDOCPP
#define STR_HPP_BOKUMEIDOCPP

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "base.hpp"
#include "type.hpp"
#include "../3rdparty/dragonbox.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace str
{
    /*  将输入转换为std::string
        - 基于std::ostringstream实现，可以接收任意类型arg
        - 支持重载了std::ostream& operator<<(std::ostream&, const T&)的T对象，operator<<中不可再调用此toStr
        - 扩展了对非volatile限定的STL可遍历容器对象的支持
        - 有无符号的char都会被当作字符处理
        - 宽字符会被当作数字处理，wstring会被当作装有宽字符的容器（{65, 66, ...}）
        - 未支持的类型会被转换为<ClassName: Address>形式的字符串
        - float_precision为负时表示自动模式，可能会用科学计数法表示  */
    template <int8_t float_precision = -1, class T>
    std::string toStr(const T& arg);

    /*  将字符串中的"{}"替换为后续的参数
        - 基于std::ostringstream实现，可以接收任意类型arg
        - 支持重载了std::ostream& operator<<(std::ostream&, const T&)的T对象，operator<<中不可再调用此format
        - 扩展了对非volatile限定的STL可遍历容器对象的支持
        - 有无符号的char都会被当作字符处理
        - 宽字符会被当作数字处理，wstring会被当作装有宽字符的容器（{65, 66, ...}）
        - 未支持的类型会被转换为<ClassName: Address>形式的字符串
        - float_precision为负时表示自动模式，可能会用科学计数法表示  */
    template <int8_t float_precision = -1, class... Args>
    std::string format(const char* f_string, const Args&... args);

    // 将整数转换为序数词，1st、2nd等
    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    std::string ordinalize(T number);

    /*  实现正向查找sep对字符串分割的功能，以vector形式返回。分割空字符串会返回包含一个空字符串的vector
        @param s：待分割的字符串
        @param sep：分割符，不可为空字符串
        @param skip_empty：是否跳过空字符串元素
        @param max_split_times：最大分割次数，跳过空字符串操作不算分隔次数，-1代表全部分割
        @return 分割结果   */
    std::vector<std::string> split(const std::string& s, const char* sep, bool skip_empty = false, size_t split_times = -1);

    /*  实现反向查找sep对字符串分割的功能，以vector形式返回。分割空字符串会返回包含一个空字符串的vector
        @param s：待分割的字符串
        @param sep：分割符，不可为空字符串
        @param skip_empty：是否跳过空字符串元素
        @param max_split_times：最大分割次数，跳过空字符串操作不算分隔次数，-1代表全部分割
        @return 分割结果，元素排列顺序不会反向   */
    std::vector<std::string> rsplit(const std::string& s, const char* sep, bool skip_empty = false, size_t max_splits = -1);

    /*  实现对指定字符集中的任意字符的正向查找分割功能，以vector形式返回。分割空字符串会返回包含一个空字符串的vector
        @param s：待分割的字符串
        @param sep_chars：分割符集合，不可为空字符串
        @param skip_empty：是否跳过空字符串元素
        @param max_split_times：最大分割次数，跳过空字符串操作不算分隔次数，-1代表全部分割
        @return 分割结果   */
    std::vector<std::string> splitAny(const std::string& s, const char* sep_chars, bool skip_empty = false, size_t max_splits = -1);

    /*  实现对指定字符集中的任意字符的反向查找分割功能，以vector形式返回。分割空字符串会返回包含一个空字符串的vector
        @param s：待分割的字符串
        @param sep_chars：分割符集合，不可为空字符串
        @param skip_empty：是否跳过空字符串元素
        @param max_split_times：最大分割次数，跳过空字符串操作不算分隔次数，-1代表全部分割
        @return 分割结果，元素排列顺序不会反向   */
    std::vector<std::string> rsplitAny(const std::string& s, const char* sep_chars, bool skip_empty = false, size_t max_splits = -1);

    // 去除字符串首尾的指定字符
    std::string trim(std::string s, const char* chars_to_remove = " \t\n\r\f\v");
    // 去除字符串首端的指定字符
    std::string ltrim(std::string s, const char* chars_to_remove = " \t\n\r\f\v");
    // 去除字符串尾端的指定字符
    std::string rtrim(std::string s, const char* chars_to_remove = " \t\n\r\f\v");

    // 在字符串首端添加字符至目标长度
    std::string lpad(std::string s, size_t target_len, const char pad_char);
    // 在字符串尾端添加字符至目标长度
    std::string rpad(std::string s, size_t target_len, const char pad_char);

}    // namespace str










/*--------------------------------------------内部实现--------------------------------------------*/

namespace _priv
{
    // 格式输出缓冲：栈上先缓存文字段，减少 oss 虚函数调用
    class AutoOStream
    {
    public:
        AutoOStream(std::ostream* oss) : os_(oss) {}

        // 将缓冲区写入底层ostream，清空缓冲区
        void flush()
        {
            os_->rdbuf()->sputn(buf_, static_cast<std::streamsize>(pos_));
            pos_ = 0;
        }

        // 写入字符串
        void write(const char* s, size_t len)
        {
            if (pos_ + len > sizeof(buf_))
            {
                flush();
                if (len > sizeof(buf_) / 2)    // 较长字符串直接写入oss，避免拷贝
                {
                    os_->rdbuf()->sputn(s, static_cast<std::streamsize>(len));
                    return;
                }
            }
            memcpy(buf_ + pos_, s, len);
            pos_ += len;
        }

        // 写入单个字符
        void put(char c)
        {
            if (pos_ >= sizeof(buf_))
                this->flush();
            buf_[pos_++] = c;
        }

        // 将缓冲区与输入内容依次写入底层ostream，输入自动转换为字符串
        template <class T>
        std::ostream& operator<<(const T& arg)    // 返回ostream&，避免链式调用时多次flush
        {
            this->flush();
            return (*os_) << arg;
        }

        // 获取底层ostream
        std::ostream& getOs()
        { return *os_; }

    private:
        char buf_[512];
        size_t pos_ = 0;
        std::ostream* os_;
    };

    // 基于std::string的连续输出缓冲区：写入的数据直接append进string
    // TODO: 考虑是否支持外部给定string引用作为buf，实现formatTo之类的功能
    class StringOutBuf : public std::streambuf
    {
    public:
        std::string& str() { return str_; }
        const std::string& str() const { return str_; }

        // 清空内容但保留容量，配合thread_local复用避免重复分配
        void clear() { str_.clear(); }

    protected:
        virtual int_type overflow(int_type c) override
        {
            if (traits_type::eq_int_type(c, traits_type::eof()))
                return traits_type::not_eof(c);    // eof 视为刷新请求，不写入字符
            str_.push_back(traits_type::to_char_type(c));
            return c;
        }

        virtual std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            str_.append(s, static_cast<size_t>(n));
            return n;
        }

    private:
        std::string str_;
    };

    // 添加对initializer_list类型的处理
    template <class T>
    void osInput(AutoOStream& aos, std::initializer_list<T> arg);

    // 添加对array类型的处理
    template <class T, size_t n>
    void osInput(AutoOStream& aos, const std::array<T, n>& arr);

    // 添加对string类型的处理
    void osInput(AutoOStream& aos, const std::string& str);

    // 添加对map类型的处理
    template <class KeyT, class VT, class... Ts>
    void osInput(AutoOStream& aos, const std::map<KeyT, VT, Ts...>& m);

    // 添加对multimap类型的处理
    template <class KeyT, class VT, class... Ts>
    void osInput(AutoOStream& aos, const std::multimap<KeyT, VT, Ts...>& m);

    // 添加对unordered_map类型的处理
    template <class KeyT, class VT, class... Ts>
    void osInput(AutoOStream& aos, const std::unordered_map<KeyT, VT, Ts...>& m);

    // 添加对unordered_multimap类型的处理
    template <class KeyT, class VT, class... Ts>
    void osInput(AutoOStream& aos, const std::unordered_multimap<KeyT, VT, Ts...>& m);

    // 添加对pair的支持
    template <class T1, class T2>
    void osInput(AutoOStream& aos, const std::pair<T1, T2>& pa);

    // 添加对tuple的支持
    template <class... Ts>
    void osInput(AutoOStream& aos, const std::tuple<Ts...>& tp);

    // 添加对STL标准可遍历容器的处理
    template <template <class U, class... Us> class CTer, class T, class... Ts, typename std::enable_if<_priv::StdBeginEndChecker<const CTer<T, Ts...>>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const CTer<T, Ts...>& cter);

    // 添加对数组类型的处理
    template <class T, int n>
    void osInput(AutoOStream& aos, const T (&arr)[n]);

    // 判断一个类型是否为普通函数或成员函数
    template <class T>
    using IsFuncOrMemFuncChecker = std::integral_constant<bool, std::is_function<typename std::remove_pointer<const T>::type>::value || std::is_member_function_pointer<const T>::value>;

    // 判断一个指针是否属于char指针(char*、const char*、volatile char*、 const volatile char*及有无符号形式)
    template <class T>
    using PointerBelongToCharChecker = std::integral_constant<bool, type::InTypesChecker<typename std::remove_cv<typename std::remove_pointer<T>::type>::type, char, signed char, unsigned char>::value>;


    // 添加对函数指针类型的处理
    template <class T, typename std::enable_if<IsFuncOrMemFuncChecker<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对指针类型的处理
    template <class T, typename std::enable_if<!IsFuncOrMemFuncChecker<T>::value && std::is_pointer<T>::value && PointerBelongToCharChecker<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对字符串指针类型的处理
    template <class T, typename std::enable_if<!IsFuncOrMemFuncChecker<T>::value && std::is_pointer<T>::value && !PointerBelongToCharChecker<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对bool类型的处理
    void osInput(AutoOStream& aos, bool arg);

    // 添加对整数类型的处理
    template <class T, typename std::enable_if<_priv::NonBoolIntChecker<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对浮点类型的处理
    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对其他支持operator<<的类型的处理
    template <class T, typename std::enable_if<type::StdCoutEachChecker<const T>::value && !IsFuncOrMemFuncChecker<T>::value && !std::is_pointer<T>::value && !std::is_arithmetic<T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);

    // 添加对不支持operator<<的类型的处理
    template <class T, typename std::enable_if<!type::StdCoutEachChecker<const T>::value, int>::type = 0>
    void osInput(AutoOStream& aos, const T& arg);



    inline int& osInputFloatPrecision()
    {
        thread_local int precision = -1;
        return precision;
    }


    template <class T>
    void osInput(AutoOStream& aos, std::initializer_list<T> arg)
    {
        aos.put('{');
        for (auto it = arg.begin(); it != arg.end();)
        {
            _priv::osInput(aos, *it);
            it++;
            if (it != arg.end())
                aos.write(", ", 2);
        }
        aos.put('}');
    }

    // 添加对std::array类型的支持
    template <class T, size_t n>
    inline void osInput(AutoOStream& aos, const std::array<T, n>& arr)
    {
        aos.put('{');
        if (n > 0)
        {
            auto bg = arr.begin();
            for (size_t i = 0; i < n - 1; ++i)
            {
                _priv::osInput(aos, *bg);
                aos.write(", ", 2);
                ++bg;
            }
            _priv::osInput(aos, *bg);
        }
        aos.put('}');
    }

    inline void osInput(AutoOStream& aos, const std::string& str)
    {
        aos.write(str.c_str(), str.size());
    }

    // wstring直接匹配到通用标准库容器的重载
    // inline void osInput(AutoOStream& aos, const std::wstring& str)
    //{
    //     oss << "<" << type::getTypeName<std::wstring>() << ": " << std::showbase << std::hex << uintptr_t(&str) << std::dec << ">";
    // }

    // 添加对std::pair类型的支持
    template <class T1, class T2>
    inline void osInput(AutoOStream& aos, const std::pair<T1, T2>& pa)
    {
        aos.put('{');
        _priv::osInput(aos, pa.first);
        aos.write(", ", 2);
        _priv::osInput(aos, pa.second);
        aos.put('}');
    }


    template <size_t idx, class... Ts>
    inline void osInputTuple(AutoOStream& aos, const std::tuple<Ts...>&, std::false_type)
    {
        aos.put('}');
    }

    template <size_t idx, class... Ts>
    inline void osInputTuple(AutoOStream& aos, const std::tuple<Ts...>& tp, std::true_type)
    {
        _priv::osInput(aos, std::get<idx>(tp));
        if (idx < sizeof...(Ts) - 1)
            aos.write(", ", 2);
        _priv::osInputTuple<idx + 1, Ts...>(aos, tp, std::integral_constant<bool, (idx + 1 < sizeof...(Ts))>());
    }

    // 添加对std::tuple类型的支持
    template <class... Ts>
    inline void osInput(AutoOStream& aos, const std::tuple<Ts...>& tp)
    {
        aos.put('{');
        _priv::osInputTuple<0, Ts...>(aos, tp, std::integral_constant<bool, (0 < sizeof...(Ts))>());
    }


    template <template <class KeyT, class VT, class... Ts> class CTer, class KeyT, class VT, class... Ts>
    inline void osInputMap(AutoOStream& aos, const CTer<KeyT, VT, Ts...>& m)
    {
        aos.put('{');
        size_t size = m.size();
        if (size > 0)
        {
            auto it = m.begin();
            for (size_t i = 0; i < size - 1; ++i)
            {
                _priv::osInput(aos, it->first);
                aos.put(':');
                _priv::osInput(aos, it->second);
                aos.write(", ", 2);
                ++it;
            }
            _priv::osInput(aos, it->first);
            aos.put(':');
            _priv::osInput(aos, it->second);
        }
        aos.put('}');
    }

    // 添加对std::map类型的支持
    template <class KeyT, class VT, class... Ts>
    inline void osInput(AutoOStream& aos, const std::map<KeyT, VT, Ts...>& m)
    {
        _priv::osInputMap(aos, m);
    }

    // 添加对std::multimap类型的支持
    template <class KeyT, class VT, class... Ts>
    inline void osInput(AutoOStream& aos, const std::multimap<KeyT, VT, Ts...>& m)
    {
        _priv::osInputMap(aos, m);
    }

    // 添加对std::unordered_map类型的支持
    template <class KeyT, class VT, class... Ts>
    inline void osInput(AutoOStream& aos, const std::unordered_map<KeyT, VT, Ts...>& m)
    {
        _priv::osInputMap(aos, m);
    }

    // 添加对std::unordered_multimap类型的支持
    template <class KeyT, class VT, class... Ts>
    inline void osInput(AutoOStream& aos, const std::unordered_multimap<KeyT, VT, Ts...>& m)
    {
        _priv::osInputMap(aos, m);
    }

    // 添加对STL标准容器的支持
    template <template <class U, class... Us> class CTer, class T, class... Ts, typename std::enable_if<_priv::StdBeginEndChecker<const CTer<T, Ts...>>::value, int>::type>
    inline void osInput(AutoOStream& aos, const CTer<T, Ts...>& cter)    // 虽然用了双层模板，但单层也可以达成目的
    {
        aos.put('{');
        for (auto it = cter.begin(); it != cter.end();)
        {
            _priv::osInput(aos, *it);
            it++;
            if (it != cter.end())
                aos.write(", ", 2);
        }
        aos.put('}');
    }

    // 添加对数组类型的支持
    template <class T, int n>
    inline void osInput(AutoOStream& aos, const T (&arr)[n])
    {
        if (type::InTypesChecker<T, char, signed char, unsigned char>::value)
            aos.write((const char*)arr, n - 1);
        else if (type::InTypesChecker<T, volatile char, volatile signed char, volatile unsigned char>::value)
        {
            for (int i = 0; i < n - 1; i++)
            {
                _priv::osInput(aos, arr[i]);
            }
        }
        else
        {
            aos.put('{');
            for (int i = 0; i < n - 1; i++)
            {
                _priv::osInput(aos, arr[i]);
                aos.write(", ", 2);
            }
            _priv::osInput(aos, arr[n - 1]);
            aos.put('}');
        }
    }

    // 添加对函数及函数指针的支持
    template <class T, typename std::enable_if<IsFuncOrMemFuncChecker<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T&)
    {
        const std::string& type_name = type::getTypeName<T>();
        aos.write(type_name.c_str(), type_name.size());
    }

    // 添加对char指针的支持
    template <class T, typename std::enable_if<!IsFuncOrMemFuncChecker<T>::value && std::is_pointer<T>::value && PointerBelongToCharChecker<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        if (!arg)
            aos.write("nullptr", 7);
        else if (type::InTypesChecker<typename std::remove_const<typename std::remove_pointer<T>::type>::type, char, signed char, unsigned char>::value)
            aos.write((const char*)arg, strlen((const char*)arg));    // 普通字符串
        else                                                          // volatile char*就一个字符一个字符地传入
        {
            for (int i = 0; arg[i] != '\0'; i++)
            {
                aos.put(char(arg[i]));
            }
        }
    }

    // 添加对其他指针的支持
    template <class T, typename std::enable_if<!IsFuncOrMemFuncChecker<T>::value && std::is_pointer<T>::value && !PointerBelongToCharChecker<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        aos << std::showbase << std::hex << uintptr_t(arg) << std::dec;
    }

    // 整数→字符缓冲区公共辅助：将无符号整数值写入 buf 末尾，返回起始下标
    // 调用方通过 buf+pos, bufsz-pos 获取结果字符串
    template <class UT>
    inline size_t intToBuf(char* buf, size_t buf_sz, UT uval, bool negative) noexcept
    {
        size_t pos = buf_sz;
        while (uval >= 100)
        {
            size_t pair = static_cast<size_t>(uval % 100);
            uval = static_cast<UT>(uval / 100);
            const char* d = digits2(pair);
            buf[--pos] = d[1];
            buf[--pos] = d[0];
        }
        if (uval >= 10)
        {
            const char* d = digits2(static_cast<size_t>(uval));
            buf[--pos] = d[1];
            buf[--pos] = d[0];
        }
        else
        {
            buf[--pos] = static_cast<char>('0' + static_cast<int>(uval));
        }
        if (negative) buf[--pos] = '-';
        return pos;
    }

    inline void osInput(AutoOStream& aos, bool arg)
    {
        aos.put(arg ? '1' : '0');
    }

    template <class T, typename std::enable_if<_priv::NonBoolIntChecker<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        if (type::InTypesChecker<T, char, signed char, unsigned char, volatile char, volatile signed char, volatile unsigned char>::value)
        {
            aos.put(char(arg));
            return;
        }

        if (arg == 0)
        {
            aos.put('0');
            return;
        }

        using UT = typename std::make_unsigned<T>::type;
        constexpr size_t buf_sz = std::numeric_limits<T>::digits10 + 2;
        char buf[buf_sz];

        bool negative = (arg < 0);
        UT uval = negative ? static_cast<UT>(-(arg + 1)) + UT(1) : static_cast<UT>(arg);

        size_t pos = _priv::intToBuf(buf, buf_sz, uval, negative);
        aos.write(buf + pos, buf_sz - pos);
    }


    template <class T, typename std::enable_if<type::InTypesChecker<T, float, volatile float, double, volatile double>::value, int>::type = 0>
    inline size_t snprintfFloating(char* buf, size_t buf_sz, T arg, int fp)
    {
        size_t len = -1;
        if (fp >= 0)
            len = snprintf(buf, buf_sz, "%.*f", fp, arg);
        else
            len = snprintf(buf, buf_sz, "%g", arg);
        return len;
    }

    template <class T, typename std::enable_if<type::InTypesChecker<T, long double, volatile long double>::value, int>::type = 0>
    inline size_t snprintfFloating(char* buf, size_t buf_sz, T arg, int fp)
    {
        // 只处理float、double、long double，如果加入了其他浮点类型，则返回失败标志
        size_t len = -1;
        if (fp >= 0)
            len = snprintf(buf, buf_sz, "%.*Lf", fp, arg);
        else
            len = snprintf(buf, buf_sz, "%Lg", arg);
        return len;
    }

    // 未来新增的浮点类型，返回失败标志
    template <class...>
    inline size_t snprintfFloating(...)
    {
        return -1;
    }

    // 分流浮点数处理，符合要求的走DragonBox，否则走snprintfFloating
    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline size_t floatToChars(char* buf, size_t buf_sz, T arg, int fp)
    {
        // 满足IEEE 754标准时，走DragonBox
        // fp<0 时用自动模式（自由的），fp>=0 时用定点格式（纯小数，精确到fp位）
        if (sizeof(T) == 4 && sizeof(float) == 4 && std::numeric_limits<T>::is_iec559)
        {
            if (fp < 0)
                return _dbox::toChars(static_cast<float>(arg), buf, buf_sz, _dbox::FloatFormat::AUTOMATIC);
            else
                return _dbox::toChars(static_cast<float>(arg), buf, buf_sz, _dbox::FloatFormat::FIXED, fp);
        }
        if (sizeof(T) == 8 && sizeof(double) == 8 && std::numeric_limits<T>::is_iec559)
        {
            if (fp < 0)
                return _dbox::toChars(static_cast<double>(arg), buf, buf_sz, _dbox::FloatFormat::AUTOMATIC);
            else
                return _dbox::toChars(static_cast<double>(arg), buf, buf_sz, _dbox::FloatFormat::FIXED, fp);
        }
        // 不满足的走标准snprintf
        return snprintfFloating(buf, buf_sz, arg, fp);
    }


    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        int fp = _priv::osInputFloatPrecision();
        char buf[64];
        size_t len = _priv::floatToChars(buf, sizeof(buf), arg, fp);
        if (len < sizeof(buf))
            aos.write(buf, len);
        else if (fp >= 0)
            aos << std::fixed << std::setprecision(fp) << arg;
        else
            aos << std::defaultfloat << std::setprecision(6) << arg;
    }

    // 添加对其他支持operator<<的类型的支持
    template <class T, typename std::enable_if<type::StdCoutEachChecker<const T>::value && !IsFuncOrMemFuncChecker<T>::value && !std::is_pointer<T>::value && !std::is_arithmetic<T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        aos << arg;
    }

    // 扩展其他不支持operator<<的类型
    template <class T, typename std::enable_if<!type::StdCoutEachChecker<const T>::value, int>::type>
    inline void osInput(AutoOStream& aos, const T& arg)
    {
        aos.put('<');
        const std::string& type_name = type::getTypeName<T>();
        aos.write(type_name.c_str(), type_name.size());
        aos.put(':');
        aos << std::showbase << std::hex << uintptr_t(&arg) << std::dec;
        aos.put('>');
    }

    inline std::ostringstream& getToStrOss()
    {
        // QNX710的gcc8.3.0似乎没法正确处理子线程的thread_local std::ostringstream对象，只能用指针来管理
        thread_local std::unique_ptr<std::ostringstream> oss_ptr(new std::ostringstream);
        return *oss_ptr;
    }

    // 整数类型：通过 intToBuf 查表直接构造 string
    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    inline std::string toStrDispatch(const T& arg)
    {
        if (arg == 0)
            return std::string("0");

        using UT = typename std::make_unsigned<T>::type;
        constexpr size_t buf_sz = std::numeric_limits<T>::digits10 + 2;
        char buf[buf_sz];

        bool negative = (arg < 0);
        UT uval = negative ? static_cast<UT>(-(arg + 1)) + UT(1) : static_cast<UT>(arg);

        size_t pos = _priv::intToBuf(buf, buf_sz, uval, negative);
        return std::string(buf + pos, buf_sz - pos);
    }

    // 其他浮点→ snprintf 回退
    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline std::string toStrDispatch(T arg)
    {
        int fp = _priv::osInputFloatPrecision();
        char buf[64];
        size_t len = _priv::floatToChars(buf, sizeof(buf), arg, fp);
        if (len < sizeof(buf))
            return std::string(buf, len);
        std::ostringstream& oss = _priv::getToStrOss();
        oss.str(std::string());
        oss.clear();    // 复位状态位，避免异常残留的 badbit 导致后续输出静默丢失
        if (fp >= 0)
            oss << std::fixed << std::setprecision(fp);
        else
            oss << std::defaultfloat << std::setprecision(6);
        oss << arg;
        return oss.str();
    }

    // 非算术非字符串类型（容器/自定义类型等）：走 osInput 路径
    template <class T, typename std::enable_if<!std::is_arithmetic<T>::value && !type::ConstructibleFromEachChecker<std::string, const T&>::value, int>::type = 0>
    inline std::string toStrDispatch(const T& arg)
    {
        std::ostringstream& oss = _priv::getToStrOss();

        _priv::AutoOStream aos(&oss);
        oss.str(std::string());
        oss.clear();    // 复位状态位，避免异常残留的 badbit 导致后续输出静默丢失
        _priv::osInput(aos, arg);
        aos.flush();
        return oss.str();
    }

    template <class T, typename std::enable_if<type::ConstructibleFromEachChecker<std::string, const T&>::value, int>::type = 0>
    inline std::string toStrDispatch(const T& arg)
    {
        return arg;
    }


    template <class Arg>
    inline void formatOne(AutoOStream& aos, const char* s, const size_t len, size_t& pos_offset, const Arg& arg)
    {
        if (pos_offset >= len)
            return;
        const char* start_pos = s + pos_offset;
        const char* pos = strstr(start_pos, "{}");
        if (pos)
        {
            aos.write(start_pos, pos - start_pos);
            _priv::osInput(aos, arg);
            pos_offset = pos - s + 2;
        }
    }

    // 展开参数包替换所有 {} 占位符（使用数组初始化列表保证从左到右顺序求值，避免递归模板实例化膨胀）
    template <class... Args>
    inline void formatAll(AutoOStream& aos, const char* s, const size_t len, const Args&... args)
    {
        size_t pos_offset = 0;
        int tmp[] = {0, (formatOne(aos, s, len, pos_offset, args), 0)...};
        (void)tmp;
        if (pos_offset < len)
            aos.write(s + pos_offset, len - pos_offset);
    }

    inline std::ostringstream& getFormatOss()
    {
        // QNX710的gcc8.3.0似乎没法正确处理子线程的thread_local std::ostringstream对象，只能用指针来管理
        thread_local std::unique_ptr<std::ostringstream> oss_ptr(new std::ostringstream);

        return *oss_ptr;
    }

    template <class... Args>
    inline std::string formatDispatch(const char* f_string, const Args&... args)
    {
        std::ostringstream& oss = _priv::getFormatOss();
        size_t len = strlen(f_string);
        oss.str(std::string());
        oss.clear();    // 复位状态位，避免异常残留的 badbit 导致后续输出静默丢失

        AutoOStream aos(&oss);
        _priv::formatAll(aos, f_string, len, args...);
        aos.flush();
        return oss.str();
    }

    // 当没有参数时，不用oss减少一次拷贝
    inline std::string formatDispatch(const char* f_string)
    {
        return std::string(f_string);
    }

}    // namespace _priv

namespace str
{

    template <int8_t float_precision, class T>
    inline std::string toStr(const T& arg)
    {
        _priv::osInputFloatPrecision() = float_precision;
        std::string str = _priv::toStrDispatch(arg);
        _priv::osInputFloatPrecision() = -1;
        return str;
    }


    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type>
    inline std::string ordinalize(T number)
    {
        auto num = number + 0;
        using UT = typename std::make_unsigned<decltype(num)>::type;
        UT abs_num = num < 0 ? static_cast<UT>(-(num + 1)) + UT(1) : static_cast<UT>(num);
        if ((abs_num % 10 == 1) && (abs_num % 100 != 11))
            return str::toStr(num).append("st");
        if ((abs_num % 10 == 2) && (abs_num % 100 != 12))
            return str::toStr(num).append("nd");
        if ((abs_num % 10 == 3) && (abs_num % 100 != 13))
            return str::toStr(num).append("rd");
        return str::toStr(num).append("th");
    }

    template <int8_t float_precision, class... Args>
    inline std::string format(const char* f_string, const Args&... args)
    {
        if (!f_string)
        {
            _MEIDO_WARN("Param f_string is null");
            return std::string();
        }
        _priv::osInputFloatPrecision() = float_precision;
        std::string str = _priv::formatDispatch(f_string, args...);
        _priv::osInputFloatPrecision() = -1;
        return str;
    }


    inline std::vector<std::string> split(const std::string& s, const char* sep, bool skip_empty, size_t max_splits)
    {
        if (s.empty())
            return skip_empty ? std::vector<std::string>{} : std::vector<std::string>{s};
        if (max_splits == 0)
            return {s};
        if (!sep || sep[0] == '\0')
        {
            _MEIDO_WARN("null or empty sep is invalid");
            return {s};
        }
        size_t sep_len = strlen(sep);
        std::vector<std::string> strs;
        strs.reserve(16);    // 估计值

        size_t now_counts = 0;
        size_t start_pos = 0;
        size_t sep_pos;
        while (now_counts < max_splits)
        {
            sep_pos = s.find(sep, start_pos, sep_len);
            if (sep_pos == std::string::npos)
                break;
            // 如果子字符串不为空，或者为空时不跳过，那么将其加入结果
            if (sep_pos != start_pos || !skip_empty)
            {
                strs.emplace_back(s.substr(start_pos, sep_pos - start_pos));
                now_counts++;
            }
            start_pos = sep_pos + sep_len;
        }
        if (start_pos < s.length() || !skip_empty)
            strs.emplace_back(s.substr(start_pos));
        return strs;
    }

    inline std::vector<std::string> rsplit(const std::string& s, const char* sep, bool skip_empty, size_t max_splits)
    {
        if (s.empty())
            return skip_empty ? std::vector<std::string>{} : std::vector<std::string>{s};
        if (max_splits == 0)
            return {s};
        if (!sep || sep[0] == '\0')
        {
            _MEIDO_WARN("null or empty sep is invalid");
            return {s};
        }
        size_t sep_len = strlen(sep);
        std::vector<std::string> strs;
        std::vector<std::string> temp_strs;
        temp_strs.reserve(16);

        size_t now_counts = 0;
        size_t start_pos = s.size() - 1;         // 理论查找起始位置
        size_t find_start_pos = s.size() - 1;    // rfind查找多字节字符串时，只需要首位出现在[0, pos]中就会找到，因此做一个实际偏移后的查找起始位置
        size_t sep_pos = std::string::npos;
        size_t tmp_sep_pos;
        while (now_counts < max_splits && sep_pos != 0)
        {
            tmp_sep_pos = s.rfind(sep, find_start_pos, sep_len);
            if (tmp_sep_pos == std::string::npos)
                break;
            // 如果分隔符的末位坐标超过了理论反向查找的起始位置，则该次查找应被忽略
            if (tmp_sep_pos + sep_len - 1 > start_pos)
            {
                if (find_start_pos == 0)
                    break;
                find_start_pos--;
                continue;
            }

            sep_pos = tmp_sep_pos;
            size_t substr_len = start_pos - sep_pos + 1 - sep_len;    // 坐标相减再加1代表子串长度
            if (substr_len != 0 || !skip_empty)
            {
                temp_strs.emplace_back(s.substr(sep_pos + sep_len, substr_len));
                now_counts++;
            }
            start_pos = sep_pos - 1;
            find_start_pos = sep_pos - 1;
        }

        if (sep_pos != 0 || !skip_empty)
            temp_strs.emplace_back(s.substr(0, sep_pos));
        strs.reserve(temp_strs.size());
        for (auto it = temp_strs.rbegin(); it != temp_strs.rend(); ++it)
            strs.push_back(std::move(*it));
        return strs;
    }


    inline std::vector<std::string> splitAny(const std::string& s, const char* sep_chars, bool skip_empty, size_t max_splits)
    {
        if (s.empty())
            return skip_empty ? std::vector<std::string>{} : std::vector<std::string>{s};
        if (max_splits == 0)
            return {s};
        if (!sep_chars || sep_chars[0] == '\0')
        {
            _MEIDO_WARN("null or empty sep_chars is invalid");
            return {s};
        }
        size_t sep_len = strlen(sep_chars);
        std::vector<std::string> strs;
        strs.reserve(16);    // 估计值

        size_t now_counts = 0;
        size_t start_pos = 0;
        size_t sep_pos;
        while (now_counts < max_splits)
        {
            sep_pos = s.find_first_of(sep_chars, start_pos, sep_len);
            if (sep_pos == std::string::npos)
                break;

            if (sep_pos != start_pos || !skip_empty)
            {
                strs.emplace_back(s.substr(start_pos, sep_pos - start_pos));
                now_counts++;
            }
            start_pos = sep_pos + 1;
        }
        if (start_pos < s.length() || !skip_empty)
            strs.emplace_back(s.substr(start_pos));
        return strs;
    }

    inline std::vector<std::string> rsplitAny(const std::string& s, const char* sep_chars, bool skip_empty, size_t max_splits)
    {
        if (s.empty())
            return skip_empty ? std::vector<std::string>{} : std::vector<std::string>{s};
        if (max_splits == 0)
            return {s};
        if (!sep_chars || sep_chars[0] == '\0')
        {
            _MEIDO_WARN("null or empty sep_chars is invalid");
            return {s};
        }
        size_t sep_len = strlen(sep_chars);
        std::vector<std::string> strs;
        std::vector<std::string> temp_strs;
        temp_strs.reserve(16);
        size_t now_counts = 0;
        size_t start_pos = s.size() - 1;    // 理论查找起始位置
        size_t sep_pos = std::string::npos;
        size_t temp_sep_pos;
        while (now_counts < max_splits && sep_pos != 0)
        {
            temp_sep_pos = s.find_last_of(sep_chars, start_pos, sep_len);
            if (temp_sep_pos == std::string::npos)
                break;
            sep_pos = temp_sep_pos;
            size_t substr_len = start_pos - sep_pos;
            if (substr_len != 0 || !skip_empty)
            {
                temp_strs.emplace_back(s.substr(sep_pos + 1, substr_len));
                now_counts++;
            }
            start_pos = sep_pos - 1;
        }

        if (sep_pos != 0 || !skip_empty)
            temp_strs.emplace_back(s.substr(0, sep_pos));

        strs.reserve(temp_strs.size());
        for (auto it = temp_strs.rbegin(); it != temp_strs.rend(); ++it)
            strs.push_back(std::move(*it));
        return strs;
    }


    inline std::string trim(std::string s, const char* chars_to_remove)
    {
        if (s.empty())
            return s;
        if (!chars_to_remove || chars_to_remove[0] == '\0')
        {
            _MEIDO_WARN("null or empty chars_to_remove is invalid");
            return s;
        }

        size_t chars_len = strlen(chars_to_remove);
        size_t left_pos = s.find_first_not_of(chars_to_remove, 0, chars_len);
        if (left_pos == std::string::npos)
            s.clear();
        else
        {
            s.erase(0, left_pos);
            size_t right_pos = s.find_last_not_of(chars_to_remove, std::string::npos, chars_len);
            s.erase(right_pos + 1, s.size());
        }
        return s;
    }

    inline std::string ltrim(std::string s, const char* chars_to_remove)
    {
        if (s.empty())
            return s;
        if (!chars_to_remove || chars_to_remove[0] == '\0')
        {
            _MEIDO_WARN("null or empty chars_to_remove is invalid");
            return s;
        }

        size_t left_pos = s.find_first_not_of(chars_to_remove);
        if (left_pos == std::string::npos)
            s.clear();
        else
            s.erase(0, left_pos);
        return s;
    }

    inline std::string rtrim(std::string s, const char* chars_to_remove)
    {
        if (s.empty())
            return s;
        if (!chars_to_remove || chars_to_remove[0] == '\0')
        {
            _MEIDO_WARN("null or empty chars_to_remove is invalid");
            return s;
        }

        size_t right_pos = s.find_last_not_of(chars_to_remove);
        if (right_pos == std::string::npos)
            s.clear();
        else
            s.erase(right_pos + 1, s.size());
        return s;
    }


    inline std::string lpad(std::string s, size_t target_len, const char pad_char)
    {
        if (s.size() < target_len)
            s.insert(0, target_len - s.size(), pad_char);
        return s;
    }

    inline std::string rpad(std::string s, size_t target_len, const char pad_char)
    {
        if (s.size() < target_len)
            s.insert(s.size(), target_len - s.size(), pad_char);
        return s;
    }
}    // namespace str
}    // namespace meido

#endif    // !STR_HPP_BOKUMEIDOCPP
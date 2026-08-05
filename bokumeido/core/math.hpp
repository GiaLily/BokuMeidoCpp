/*  bokumeidocpp库的数学相关功能 */
#pragma once
#ifndef MATH_HPP_BOKUMEIDOCPP
#define MATH_HPP_BOKUMEIDOCPP

#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <math.h>

#include "base.hpp"
#include "str.hpp"
#include "type.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/
namespace math
{
    // 避免整型溢出的安全绝对值函数，可接受bool外的整型输入
    template <class T, typename std::enable_if<_priv::NonBoolIntChecker<T>::value, int>::type = 0>
    constexpr typename std::make_unsigned<T>::type safeAbs(T x);


    /*  将val约束至[lo, hi]范围内
        - 当val < lo时返回lo
        - 当val > hi时返回hi
        - 否则返回val
        使用三元比较实现，仅涉及比较和拷贝，无溢出风险
        - 当lo > hi时自动交换边界，保证[lo, hi]区间有效
        @param val: 待约束的值
        @param lo: 范围下限
        @param hi: 范围上限
        @return 约束后的值   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type = 0>
    constexpr T clamp(T val, T lo, T hi);

    /*  符号函数，判断val的正负性
        - 正数返回1
        - 零返回0
        - 负数返回-1
        - 对于无符号类型，返回值仅可能为{0, 1}
        通过两个布尔比较相减实现，结果恒在[-1, 1]范围内，无溢出风险
        @param val: 输入值
        @return int类型，-1、0或1   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type = 0>
    constexpr int sign(T val);

    /*  判断正整数是否为2的幂
        - 正整数且二进制表示中仅有一位为1时返回true
        - 0和负数返回false
        通过val & (val - 1)位运算判断，无溢出风险
        @param val: 待检查的整数
        @return 是2的幂返回true，否则返回false   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_integral<T>::value, int>::type = 0>
    constexpr bool isPowerOfTwo(T val);


    /*  圆周率π
        @tparam T: 返回值的算术类型，整数类型时被截断   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type = 0>
    constexpr T pi();

    /*  自然常数e
        @tparam T: 返回值的算术类型，整数类型时被截断   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type = 0>
    constexpr T e();

    /*  黄金分割比例φ
        @tparam T: 返回值的算术类型，整数类型时被截断   */
    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type = 0>
    constexpr T phi();

    // 矩形类
    template <class T>
    class Rect final
    {
    public:
        // 通过两个点构造矩形，内部会自动调整坐标顺序
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && !type::AnyTopCVRefChecker<U>::value && std::is_arithmetic<U>::value, int>::type = 0>
        Rect(T x1, T y1, T x2, T y2);

        Rect(const Rect<T>& rect) = default;
        Rect<T>& operator=(const Rect<T>& rect) = default;

        bool operator==(const Rect<T>& rect) const;
        bool operator!=(const Rect<T>& rect) const;

        // 获取坐标
        T xMin() const;
        T yMin() const;
        T xMax() const;
        T yMax() const;

        // 判断矩形是否包含指定的坐标点（边界上的点也算包含）
        bool contains(T x, T y) const;

        // 判断本矩形是否完全包含另一个矩形（边界重叠也算包含）
        bool contains(const Rect<T>& other) const;

        /*  求两个矩形的交集
            @param other: 另一个矩形
            @param dst: 用于写入交集结果，仅在前置条件成立时被写入
            @return 有交集时返回true，dst为交集区域；无交集时返回false，dst保持不变   */
        bool clipTo(const Rect<T>& other, Rect<T>& dst) const;

    private:
        std::array<T, 4> data_;
    };


    // 为operator<<添加对Rect的支持
    template <class T>
    std::ostream& operator<<(std::ostream& os_obj, const Rect<T>& rect);



}    // namespace math










/*--------------------------------------------内部实现--------------------------------------------*/

namespace math
{
    template <class T, typename std::enable_if<_priv::NonBoolIntChecker<T>::value, int>::type>
    inline constexpr typename std::make_unsigned<T>::type safeAbs(T x)
    {
        return _priv::safeAbs(x);
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type>
    inline constexpr T clamp(T val, T lo, T hi)
    {
        return lo > hi ? (val < hi ? hi : (lo < val ? lo : val)) : (val < lo ? lo : (hi < val ? hi : val));
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type>
    inline constexpr int sign(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_integral<T>::value, int>::type>
    inline constexpr bool isPowerOfTwo(T val)
    {
        return val > 0 && (val & (val - 1)) == 0;
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type>
    inline constexpr T pi()
    {
        return T(3.141592653589793238462643383279502884197);
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type>
    inline constexpr T e()
    {
        return T(2.718281828459045235360287471352662497757);
    }

    template <class T, typename std::enable_if<!type::AnyTopCVRefChecker<T>::value && std::is_arithmetic<T>::value, int>::type>
    inline constexpr T phi()
    {
        return T(1.618033988749894848204586834365638117720);
    }

    template <class T>
    template <class U, typename std::enable_if<std::is_same<U, T>::value && !type::AnyTopCVRefChecker<U>::value && std::is_arithmetic<U>::value, int>::type>
    inline Rect<T>::Rect(T x1, T y1, T x2, T y2)
    {
        if (x1 <= x2)
        {
            data_[0] = x1;
            data_[2] = x2;
        }
        else
        {
            data_[0] = x2;
            data_[2] = x1;
        }
        if (y1 <= y2)
        {
            data_[1] = y1;
            data_[3] = y2;
        }
        else
        {
            data_[1] = y2;
            data_[3] = y1;
        }
    }


    template <class T>
    inline bool Rect<T>::operator==(const Rect<T>& rect) const
    {
        return data_ == rect.data_;
    }

    template <class T>
    inline bool Rect<T>::operator!=(const Rect<T>& rect) const
    {
        return data_ != rect.data_;
    }

    template <class T>
    inline T Rect<T>::xMin() const
    {
        return data_[0];
    }

    template <class T>
    inline T Rect<T>::yMin() const
    {
        return data_[1];
    }

    template <class T>
    inline T Rect<T>::xMax() const
    {
        return data_[2];
    }

    template <class T>
    inline T Rect<T>::yMax() const
    {
        return data_[3];
    }

    template <class T>
    inline bool Rect<T>::clipTo(const Rect<T>& other, Rect<T>& dst) const
    {
        T u0 = std::max(data_[0], other.data_[0]);
        T u1 = std::min(data_[2], other.data_[2]);
        T v0 = std::max(data_[1], other.data_[1]);
        T v1 = std::min(data_[3], other.data_[3]);
        if (u0 > u1 || v0 > v1)
            return false;
        dst.data_[0] = u0;
        dst.data_[1] = v0;
        dst.data_[2] = u1;
        dst.data_[3] = v1;

        return true;
    }

    template <class T>
    inline bool Rect<T>::contains(T x, T y) const
    {
        return data_[0] <= x && x <= data_[2] && data_[1] <= y && y <= data_[3];
    }

    template <class T>
    inline bool Rect<T>::contains(const Rect<T>& other) const
    {
        return data_[0] <= other.data_[0] && other.data_[2] <= data_[2]
               && data_[1] <= other.data_[1] && other.data_[3] <= data_[3];
    }
}    // namespace math
namespace _priv
{

    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    inline std::ostream& coutDispatch(std::ostream& os_obj, const math::Rect<T>& rect)
    {
        // intToBuf 从缓冲区末尾向左写，利用此特性从右到左组装完整字符串
        constexpr size_t buf_sz = (std::numeric_limits<T>::digits10 + 2) * 4 + 5;    // 4个最大整数长度 + 其他间隔字符
        char buf[buf_sz];

        size_t end = sizeof(buf);    // 可用空间的右边界外1位
        buf[--end] = ']';            // 结尾括号
        end = _priv::intToBuf(buf, end, _priv::safeAbs(rect.yMax()), rect.yMax() < 0);
        buf[--end] = ' ';
        end = _priv::intToBuf(buf, end, _priv::safeAbs(rect.xMax()), rect.xMax() < 0);
        buf[--end] = ' ';
        end = _priv::intToBuf(buf, end, _priv::safeAbs(rect.yMin()), rect.yMin() < 0);
        buf[--end] = ' ';
        end = _priv::intToBuf(buf, end, _priv::safeAbs(rect.xMin()), rect.xMin() < 0);
        buf[--end] = '[';

        os_obj.write(buf + end, sizeof(buf) - end);
        return os_obj;
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline std::ostream& coutDispatch(std::ostream& os_obj, const math::Rect<T>& rect)
    {
        _priv::AutoOStream aos(&os_obj);
        aos.put('[');
        _priv::osInput(aos, rect.xMin());
        aos.put(' ');
        _priv::osInput(aos, rect.yMin());
        aos.put(' ');
        _priv::osInput(aos, rect.xMax());
        aos.put(' ');
        _priv::osInput(aos, rect.yMax());
        aos.put(']');
        aos.flush();
        return os_obj;
    }
}    // namespace _priv

namespace math
{

    template <class T>
    inline std::ostream& operator<<(std::ostream& os_obj, const Rect<T>& rect)
    {
        return _priv::coutDispatch(os_obj, rect);
    }

}    // namespace math
}    // namespace meido

#endif    // !MATH_HPP_BOKUMEIDOCPP
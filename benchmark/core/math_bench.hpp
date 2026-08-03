/*  math 模块性能测试
    测试内容：
    - clamp / sign / isPowerOfTwo（constexpr，应被优化为常量）
    - pi / e / phi（constexpr）
    - Rect 构造/contains/clipTo 运行时开销
*/
#pragma once
#ifndef BENCH_MATH_HPP
#define BENCH_MATH_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"

using namespace meido;

namespace _meidomathbench
{

inline void check()
{
    BENCH_MODULE("math 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. constexpr 函数（编译器可能完全消除，输出带常量折叠标注） ----
    {
        BENCH_N_CONSTEXPR("clamp(50, 0, 100) (constexpr)", {
            volatile auto r = math::clamp(50, 0, 100);
            (void)r;
        });
        BENCH_N_CONSTEXPR("sign(-42) (constexpr)", {
            volatile auto r = math::sign(-42);
            (void)r;
        });
        BENCH_N_CONSTEXPR("isPowerOfTwo(1024) (constexpr)", {
            volatile auto r = math::isPowerOfTwo(1024);
            (void)r;
        });
        BENCH_N_CONSTEXPR("pi<double>() (constexpr)", {
            volatile auto r = math::pi<double>();
            (void)r;
        });
    }
    BENCH_SEP();

    // ---- 2. Rect 构造 ----
    {
        BENCH_N("Rect<int> 构造 (x1,x2,y1,y2)", {
            math::Rect<int> r(10, 20, 30, 40);
            volatile auto sink = r.xMin();
            (void)sink;
        });
    }
    BENCH_SEP();

    // ---- 3. Rect contains（点） ----
    {
        math::Rect<int> r(0, 100, 0, 100);
        BENCH_N("Rect<int>::contains(x, y) 点包含", {
            volatile bool b = r.contains(50, 50);
            (void)b;
        });
    }
    BENCH_SEP();

    // ---- 4. Rect contains（矩形） ----
    {
        math::Rect<int> r(0, 100, 0, 100);
        math::Rect<int> inner(10, 90, 10, 90);
        BENCH_N("Rect<int>::contains(Rect) 矩形包含", {
            volatile bool b = r.contains(inner);
            (void)b;
        });
    }
    BENCH_SEP();

    // ---- 5. Rect clipTo（交集） ----
    {
        math::Rect<int> r1(0, 100, 0, 100);
        math::Rect<int> r2(50, 150, 50, 150);
        math::Rect<int> dst(0, 0, 0, 0);
        BENCH_N("Rect<int>::clipTo 求交集", {
            bool ok = r1.clipTo(r2, dst);
            volatile auto sink = ok;
            (void)sink;
        });
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidomathbench

#endif // BENCH_MATH_HPP

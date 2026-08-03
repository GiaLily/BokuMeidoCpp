#!/bin/bash
# BokuMeidoCpp 性能测试交叉编译脚本 (RK3588 ARM64)
# 用法: ./build_cross.sh          # Release 模式（O2，含调试符号）
#       ./build_cross.sh -d       # Debug 模式
#
# 说明: benchmark 已改为时间驱动（~1s/项），慢平台自动运行更少次数

CROSS_PATH="/home/cidi/tools/rk3588_linux_host/buildroot/output/rockchip_rk3588/host/bin/aarch64-buildroot-linux-gnu-"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 编译选项（与 build_gcc.sh 保持一致，但移除 rpath-link 等本地路径）
CXXFLAGS="-I $SCRIPT_DIR/../ -I $SCRIPT_DIR -lpthread -lglog --std=c++11 -O3 -g -fno-omit-frame-pointer"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused"
CXXFLAGS="$CXXFLAGS -Wl,--enable-new-dtags"

if [ "$1" == "-d" ]; then
    CXXFLAGS="-I $SCRIPT_DIR/../ -I $SCRIPT_DIR -lpthread -lglog --std=c++11 -O0 -g -fno-omit-frame-pointer"
    CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused"
    CXXFLAGS="$CXXFLAGS -Wl,--enable-new-dtags"
fi

echo "============================================"
echo "  BokuMeidoCpp Benchmark - Cross Compile"
echo "  Target: RK3588 (aarch64)"
echo "  Compiler: ${CROSS_PATH}g++"
echo "============================================"

"${CROSS_PATH}g++" "$SCRIPT_DIR/bokumeido_benchmark.cpp" \
    -o "$SCRIPT_DIR/bokumeido_benchmark" \
    $CXXFLAGS

if [ $? -eq 0 ]; then
    echo ""
    echo "Build SUCCESS: $SCRIPT_DIR/bokumeido_benchmark"
    "${CROSS_PATH}file" "$SCRIPT_DIR/bokumeido_benchmark"
else
    echo ""
    echo "Build FAILED"
    exit 1
fi

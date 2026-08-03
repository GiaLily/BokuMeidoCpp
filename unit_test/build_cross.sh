#!/bin/bash
# BokuMeidoCpp 单元测试交叉编译脚本 (RK3588 ARM64)
# 用法: ./build_cross.sh          # release 模式
#       ./build_cross.sh -d       # debug 模式

CROSS_PATH="/home/cidi/tools/rk3588_linux_host/buildroot/output/rockchip_rk3588/host/bin/aarch64-buildroot-linux-gnu-"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ "$1" == "-d" ]; then
    "${CROSS_PATH}g++" "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test_cross" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -g \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags
else
    "${CROSS_PATH}g++" "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test_cross" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -O3 -DNDEBUG -D_GLIBCXX_ASSERTIONS \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags
fi

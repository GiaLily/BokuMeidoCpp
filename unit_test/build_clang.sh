#!/bin/bash
# BokuMeidoCpp 单元测试编译脚本 (Clang)
# 用法: ./build_clang.sh       # release 模式
#       ./build_clang.sh -d    # debug 模式

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Release 模式编译
if [ "$1" == "-d" ]; then
    clang++ "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test_clang" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -g \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed
else
    clang++ "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test_clang" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -O3 -DNDEBUG -D_GLIBCXX_ASSERTIONS \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed
fi

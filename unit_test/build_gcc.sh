#!/bin/bash
# BokuMeidoCpp 单元测试编译脚本
# 用法: ./build.sh          # release 模式
#       ./build.sh -d       # debug 模式

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Release 模式编译
if [ "$1" == "-d" ]; then
    g++ "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -g \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed
else
    g++ "$SCRIPT_DIR/bokumeido_unit_test.cpp" "$SCRIPT_DIR/another.cpp" \
        -o "$SCRIPT_DIR/bokumeido_unit_test" \
        -I "$SCRIPT_DIR/../" \
        -lpthread \
        --std=c++11 -O3 -DNDEBUG -D_GLIBCXX_ASSERTIONS \
        -Wall -Wextra -Wno-unused \
        -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed
fi

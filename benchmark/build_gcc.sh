#!/bin/bash
# BokuMeidoCpp 性能测试编译脚本
# 用法: ./build.sh              # Release + 调试符号（适合 perf）
#       ./build.sh -d           # Debug 模式

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 编译选项（-fno-omit-frame-pointer 允许 perf 用 fp 模式快速采样）
CXXFLAGS="-I $SCRIPT_DIR/../ -I $SCRIPT_DIR -lpthread --std=c++11 -O3 -g -fno-omit-frame-pointer -Wall -Wextra -Wno-unused"
CXXFLAGS="$CXXFLAGS -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed"

if [ "$1" == "-d" ]; then
    CXXFLAGS="-I $SCRIPT_DIR/../ -I $SCRIPT_DIR -lpthread --std=c++11 -O0 -g -fno-omit-frame-pointer -Wall -Wextra -Wno-unused"
    CXXFLAGS="$CXXFLAGS -Wl,--enable-new-dtags,-rpath-link=./ -Wl,--as-needed"
fi

g++ "$SCRIPT_DIR/bokumeido_benchmark.cpp" \
    -o "$SCRIPT_DIR/bokumeido_benchmark" \
    $CXXFLAGS

echo "Build complete: $SCRIPT_DIR/bokumeido_benchmark"

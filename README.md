<div align="center">

# 🌸 BokuMeidoCpp

### 基于 C++ 标准库的轻量级功能扩展库

> format、print、log、线程池……**Meido**替您携带日常开发的小工具

`v1.0.0` • `C++11` • `Header-Only` • `跨平台` • `零依赖`

[![CI](https://github.com/GiaLily/BokuMeidoCpp/actions/workflows/ci.yml/badge.svg)](https://github.com/GiaLily/BokuMeidoCpp/actions/workflows/ci.yml)

[English](README_EN.md)

</div>

---

## 简介

BokuMeidoCpp 是一个基于 C++ 标准库的轻量级功能扩展库。

**C++11 开发中总会遇到一些问题**
- 想要 `format` 功能，但不想引入 `{fmt}` 库；自己随手写的性能一般，且只支持部分类型
- 想要任意类型都能用的 `toStr`，但标准库没有提供
- 想要类似 Python `print` 那样随时打印 `vector`、`map` 等容器来调试，不想在每个项目里重载 `operator<<`
- 想要开箱即用的日志，支持基本的同步/异步输出，不想一直 `printf` 和 `cout`，也不想引入复杂的日志框架
- 想用线程池或对象池，不想自己管理线程创建和同步
- 想要解析 `main` 参数，不想写那些稍显繁琐的解析逻辑
- 想要简单的时间获取、`sleep`，想要跨作用域的时间统计，在合适的时机输出日志并能全局关闭
- 标准库缺少常用的路径操作（`join`、`exists` 等），平台特定 API 又不跨平台
- 模板报错信息层层嵌套，难以定位真正的问题位置
- 总的来说，不想引入庞大的第三方库增加学习成本和环境管理负担，也不想每次遇到相同需求就重写一遍

**BokuMeidoCpp 的解法**
- `format`、`toStr`、`print`：支持任意标准库可遍历容器及其嵌套，通过 `operator<<` 极简适配自定义类型；未适配的类型不会编译报错，只会输出 `<ClassName: Address>`
- 日志：同步/异步输出到终端或文件，支持回调模式一键将库内所有日志转发到其他目标
- 线程池、对象池、`main` 参数解析、时间处理：接口明确，一看就会
- 路径操作：跨平台路径拼接、存在性判断等，避免使用平台相关的接口
- 模板类型检查：严格在调用处报错，不会让错误深埋在模板实例化链中
- 纯头文件、零外部依赖——把 `bokumeido/` 放进项目，`#include "bokumeido/core.hpp"` 即可使用所有功能

---

## 目录

- [快速开始](#快速开始)
- [模块介绍](#模块介绍)
- [简单性能参考](#简单性能参考)
- [设计思路](#设计思路)
- [兼容性声明](#兼容性声明)
- [注意事项](#注意事项)
- [模块功能示例](#模块功能示例)
- [版本信息](#版本信息)
- [测试平台](#测试平台)
- [版本更新日志](#版本更新日志)

---

## 快速开始

纯头文件、零依赖：把 `bokumeido/` 目录放进项目，一行 `include` 即可使用全部功能。

```cpp
#include "bokumeido/core.hpp"
#include <map>
#include <string>
#include <vector>

int main()
{
    std::vector<int> nums = {1, 2, 3};
    std::map<int, std::string> kv = {{1, "a"}, {2, "b"}};

    meido::io::print(nums, kv, 3.14);                          // {1, 2, 3} {1:a, 2:b} 3.14
    auto s = meido::str::toStr(nums);                          // "{1, 2, 3}"
    auto t = meido::str::format("{} × {} = {}", 6, 7, 6 * 7);  // "6 × 7 = 42"
    return 0;
}
```

编译运行，无需链接任何第三方库：

```bash
g++ -std=c++11 -O2 main.cpp -o app && ./app    # GCC / Clang
cl /std:c++14 /EHsc /O2 main.cpp              # MSVC
```

---

## 模块介绍

### 基本信息

| 文件夹 | 模块 | 功能描述 | 命名空间 |
|:------:|:----:|:---------|:--------:|
| **--** | **core.hpp** | 一键导入所有核心模块 | `meido` |
| `core` | **base.hpp** | 版本信息及基础宏定义 | `meido::base` |
| `core` | **type.hpp** | 编译期类型检查与模板约束 | `meido::type` |
| `core` | **str.hpp** | 字符串操作：toStr、format、split 等 | `meido::str` |
| `core` | **log.hpp** | 日志输出：同步/异步文件日志、控制台日志 | `meido::log` |
| `core` | **math.hpp** | 数学相关操作（矩形等） | `meido::math` |
| `core` | **datastruct.hpp** | 非标准库数据结构（循环队列等） | `meido::ds` |
| `core` | **mem.hpp** | 内存管理：对象池复用，避免频繁 new/delete | `meido::mem` |
| `core` | **path.hpp** | 路径操作：exists、listDir、join 等 | `meido::path` |
| `core` | **thread.hpp** | 线程相关：线程池、自旋锁、读写锁 | `meido::thrd` |
| `core` | **time.hpp** | 时间相关操作：计时、休眠等 | `meido::time` |
| `core` | **io.hpp** | 输入输出：print、参数解析、文件操作 | `meido::io` |

### 内嵌实现

| 文件 | 来源 | 作者 |
|:----|:-----|:----|
| `3rdparty/dragonbox.hpp` | [Dragonbox](https://github.com/jk-jeon/dragonbox) | Junekey Jeon |


### 使用方法

* 可以单独导入 `bokumeido` 下的模块，如：

```cpp
#include "bokumeido/core/time.hpp"   // 导入时间相关功能
#include "bokumeido/core/io.hpp"     // 导入输入打印相关功能
```

* 也可一键导入 `bokumeido/core.hpp`，如：

```cpp
#include "bokumeido/core.hpp"        // 一键导入基于标准库的核心功能封装
```

* 版本号定义在 `base.hpp` 中；Linux 下可通过 `strings xxx | grep version` 查找应用所使用的 BokuMeidoCpp 版本。

---

## 简单性能参考

以下为简单测试场景下的粗略结论，仅供参考。实际性能因编译器、硬件、系统负载而异。详细数据见 [`docs/benchmark-report.md`](docs/benchmark-report.md)。

**测试平台**：
- x86_64: Linux · Intel Core i7-8700 · GCC 11.4.0 · C++14*
- ARM64: Linux · RK3588 (Cortex-A76/A55) · GCC 11.3.0 (Buildroot) · C++14*

> *对比测试依赖 `{fmt}` 与 `spdlog`，需 `C++14` 编译。项目本身基于 `C++11` 实现。

### `str::format`

- 整体与 `{fmt}` 处于同一量级、互有胜负：浮点、长字符串等场景在 GCC 系测试平台上略有优势（得益于 Dragonbox 算法），整数等场景在部分平台上略慢（≤1.5×）；测试场景中均明显快于 `ostringstream`（约 2~5×）
- **设计上更方便的地方**：通过 `operator<<` 适配任意自定义类型——无需像 `{fmt}` 那样编写特化的 `fmt::formatter<T>`

### `log`

- 与 `spdlog` 互有取舍：异步入队路径在测试平台上略快（约 1~5×），同步/回调路径略慢（约 1~4×，ARM 上同步模式基本持平），后台落盘基本持平——整体处于同一量级
- 支持**回调模式**，可将日志消息转发到第三方日志库
- 未经完整的工业级压力测试，在极端高频场景下建议实测验证

### `str::split`

- 测试平台上比 `Boost.StringAlgorithm` 快约 2~6×，与手写 `find`+`substr` 性能相当或略优


## 设计思路

### 设计原则

| 原则 | 说明 |
|:-----|:------|
| **平台通用** | 基于 C/C++ 标准库实现跨平台功能 |
| **类型安全** | 严格的模板类型检查，确保编译错误出现在调用入口处 |
| **轻量克制** | 接口简洁克制，专注封装高频琐碎或标准库缺失的功能，避免功能重叠 |
| **性能优化** | 在达成设计意图且不增加接口复杂度的前提下，尽可能优化性能 |
| **线程安全** | 所有函数接口保证线程安全；类接口非明确说明不保证线程安全 |
| **异常规避** | 避免主动抛出异常，除非是资源限制等情况导致的被动异常 |

### 命名规则

| 类型 | 命名规范 | 示例 |
|:-----|:---------|:-----|
| **类型** | 大驼峰 | `class ThreadPool {}` |
| **函数** | 小驼峰 | `void splitString() {}` |
| **变量** | 小写+下划线 | `int task_count;` |
| **成员变量** | 小写+下划线+下划线后缀 | `int ThreadPool::task_count_ = 0;` |
| **枚举类** | 大写+下划线 | `enum class Level { INFO, DEBUG }` |
| **宏定义** | 大写+下划线+MEIDO前缀 | `#define MEIDO_INFO(msg)` |
| **Include Guard** | 文件名大写+\_HPP\_BOKUMEIDOCPP后缀 | `#define CORE_HPP_BOKUMEIDOCPP` |

### 更新规则

> 遵循 **大版本号删改接口，中版本号添加新功能，小版本号修复和优化** 的原则；在同一大版本内废弃接口仅标记，不删除。

---

## 兼容性声明

> 大版本内接口不删除不改名，但会有下列可能导致兼容问题的更新：

### 可能的变更

| 类型 | 说明 | 示例 |
|:-----|:------|:------|
| **参数类型变更** | 为了接口的准确性，更新可能修改函数参数类型为其他兼容类型，或增加带默认值的新参数 | `void func(int a)` → `void func(short a)` 或 `void func(int a, int b = 0)` |
| **新增重载** | 为了增加新的功能，更新可能增加新的函数重载，导致函数签名出现歧义 | 新增 `void func(double)` 可能导致 `func(42)` 歧义 |
| **内存布局** | 作为纯头文件库而非二进制库，更新可能改变类和结构体的内存布局 | ⚠️ 不应依赖类和结构体的布局 |

---

## 注意事项

### 重要限制

| 限制 | 说明 |
|:-----|:------|
| **🚫 避免在具有静态存储期对象的析构函数中调用本库** | 此类对象的析构顺序在程序退出阶段未定义，而本库内部依赖静态存储期对象，可能导致不可预期行为或未定义行为 |
| **🚫 避免依赖库中类或结构体的内存布局** | 作为 Header-Only 库，内存布局随时可能变化，不应使用 `sizeof`、`memcpy` 或二进制序列化 |
| **🚫 避免使用库内以下划线开头的命名空间和宏** | 这些仅用于内部功能实现（如 `_priv` 命名空间），随时可能删改，不属于对外接口 |
| **🚫 避免将本库作为 ABI 接口** | Header‑Only 库的实现无法隐藏，天然不适合作为跨 DLL / SO 的接口 |

### 其他注意事项

| 平台/场景 | 注意事项 |
|:---------|:---------|
| **QNX710 (g++ 8.3.0)** | 编译时需要指定 `-std=gnu++11` |
| **线程安全范围** | 线程安全的类型仅保证对象普通成员函数调用之间的线程安全，构造与析构过程中不保证 |
| **编译器警告** | 出现 unused 相关警告属正常现象 |

---

## 模块功能示例

### `io.hpp` — `ArgumentParser`（命令行参数解析）

```cpp
#include "bokumeido/core/io.hpp"

int main(int argc, char* argv[])
{
    meido::io::ArgumentParser parser;

    // 定义选项：布尔选项 + 值选项
    std::vector<meido::io::BooleanOption> bool_opts = {
        {"-v", "--verbose", "输出详细信息"},
        {"-h", "--help",    "显示帮助"},
    };
    std::vector<meido::io::ValueOption> str_opts = {
        {"-o", "--output", "输出路径", "./out"},       // 默认值 ./out
        {"-l", "--level",  "日志等级", "info"},
    };

    if (parser.parse(argc, argv, bool_opts, str_opts) < 0)
        return 1;

    if (parser.getBoolOpt("--help"))
    {
        parser.logPreset();  // 自动打印帮助信息
        return 0;
    }

    bool verbose = parser.getBoolOpt("--verbose");
    std::string out_path = parser.getValueOpt("--output");
    // 用法: ./app -v -o /tmp/out --level debug
    return 0;
}
```

### `log.hpp` — 日志输出

```cpp
#include "bokumeido/core/log.hpp"

// 配置日志（调用任一 setup 函数初始化）
meido::log::setupSyncLogger(meido::log::Level::DEBUG, "/var/log/app", "myapp");
// 或异步模式：meido::log::setupAsyncLogger(...)

// 输出日志
MEIDO_INFO("server started on port {}", 8080);
MEIDO_WARN("disk usage: {}%", 85);
MEIDO_ERROR("failed to connect: {}", err_msg);

// 恢复默认日志配置
meido::log::resetLogger();

// 回调模式（转发到第三方日志库）
meido::log::setupSyncLogger(meido::log::Level::INFO, [](meido::log::Message msg) {
    third_party_log(msg.level, msg.msg);
});
MEIDO_INFO("this will be forwarded");  // 不经文件，直接进回调
```

### `str.hpp` — 字符串转换与格式化

```cpp
#include "bokumeido/core/str.hpp"

// 任意类型 → 字符串（容器、pair、tuple 等均支持）
auto s1 = meido::str::toStr(std::vector<int>{1, -2, 3});   // "{1, -2, 3}"
auto s2 = meido::str::toStr(std::map<int,std::string>{{1,"a"},{2,"b"}});
// "{1:a, 2:b}"

// 浮点精度控制
auto s3 = meido::str::toStr<2>(3.14159);   // "3.14"

// format：用 {} 占位
auto s4 = meido::str::format("{} × {} = {}", 6, 7, 6 * 7);  // "6 × 7 = 42"

// 自定义类型：重载 operator<< 即可
struct Point { int x, y; };
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}
auto s5 = meido::str::toStr(Point{3, 4});  // "(3, 4)"
```

### `time.hpp` — 计时与时间统计

```cpp
#include "bokumeido/core/time.hpp"

// 跨作用域自动计时（离开作用域时打印耗时）
{
    meido::time::TimeCounterScope<meido::time::MSEC> guard("process frame");
    process();
    // 自动输出: TimeCounterScope: process frame cost time 23ms
}

// 均值统计：每 N 次输出一次平均耗时
meido::time::MeanTimeCounter tc(10);  // 窗口 10 次
while (true) {
    tc.markStart("推理");
    infer();
    tc.markEnd("推理");
    tc.logOnWindowReached<meido::time::MSEC>();
    // 每 10 次输出: 推理 mean cost time 45 ms in 10 counts
}

// 常规计时
auto t0 = meido::time::nowSteady();
do_work();
auto cost = meido::time::nowSteady().since<meido::time::MSEC>(t0);
```

### `thread.hpp` — 线程池

```cpp
#include "bokumeido/core/thread.hpp"

int compute(int x) { return x * x; }

int main() {
    meido::thrd::ThreadPool pool(4);  // 4 个工作线程

    // 提交任务（addTask 接受无参可调用对象），返回 future
    auto fut1 = pool.addTask([] { return compute(7); });
    auto fut2 = pool.addTask([] { return compute(9); });

    // 等待并获取结果（get 返回指针）
    fut1.wait();
    const int* r1 = fut1.get();   // *r1 == 49

    const int* r2 = fut2.get();   // *r2 == 81

    // 无返回值任务——wait() 确保任务在池析构前执行完毕
    auto fut3 = pool.addTask([] { /* do something */ });
    fut3.wait();

    return 0;
}
```

### `type.hpp` — 模板类型约束

```cpp
#include "bokumeido/core/type.hpp"

// 限制 T 必须是 int、char、long long 之一
template <class T, typename std::enable_if<
    meido::type::InTypesChecker<T, int, char, long long>::value, int>::type = 0>
void func(T val) {
    // 编译通过：func(42)、func('x')、func(123LL)
    // 编译错误：func(1.5) —— 错误出现在调用处，而非模板内部
}
```

### `io.hpp` — `print`（调试打印）

```cpp
#include "bokumeido/core/io.hpp"

std::vector<int> vec = {1, 2, 3};
std::map<int, std::string> mp = {{1, "a"}, {2, "b"}};

meido::io::print(42, 3.14, "hello", vec, mp);
// 输出：42 3.14 hello {1, 2, 3} {1:a, 2:b}
// 未适配 operator<< 的类型：<ClassName: 0x7fff...>
// 线程安全：print 对每次输出内部加锁，并发调用不会产生数据竞争；
// 但多次输出之间可能被其他线程的输出插入（与混用 std::cout 同理），属正常现象
```

## 版本信息

| 项目 | 信息 |
|:-----|:------|
| **当前版本** | `1.0.0` |
| **文档更新** | `2026-08-03` |

---

## 测试平台

### Windows

| 项目 | 信息 |
|:-----|:------|
| **编译器** | MSVC 19.50 (VS 2026) |
| **编译脚本** | `unit_test/build_msvc.bat` |

### Linux

| 架构 | 编译器 | 版本 | 编译脚本 |
|:-----|:------|:-----|:---------|
| x86_64 | g++ | 11.4.0 | `unit_test/build_gcc.sh` |
| aarch64 (RK3588) | g++ (交叉编译) | 11.3.0 | `unit_test/build_cross.sh` |

---

## 版本更新日志

### v1.0.0

*2026-08-03*

1. 基于 mineutils 重新设计，规范接口，新增功能模块，优化性能；
2. 增加单元测试和性能测试。

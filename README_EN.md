<div align="center">

# 🌸 BokuMeidoCpp

### A lightweight C++ standard library extension

> format, print, log, thread pool — **Meido** keeps your daily dev tools in the apron.

`v1.0.0` • `C++11` • `Header-Only` • `Cross-Platform` • `Zero Dependencies`

[![CI](https://github.com/GiaLily/BokuMeidoCpp/actions/workflows/ci.yml/badge.svg)](https://github.com/GiaLily/BokuMeidoCpp/actions/workflows/ci.yml)

[中文文档](README.md)

</div>

---

## Introduction

BokuMeidoCpp is a lightweight C++ utility library built on the standard library.

**Common pain points in C++11 development**
- You want `format`, but don't want to pull in `{fmt}`; a quick handwritten version has mediocre performance and limited type support
- You want a universal `toStr`, but the standard library doesn't have one
- You want a Python-like `print` to dump `vector`, `map`, etc. for debugging, without overloading `operator<<` in every project
- You want a ready-to-use logger with basic sync/async output, not just `printf`/`cout` or a heavy logging framework
- You want a thread pool or object pool without manually managing threads and synchronization
- You want to parse `main()` arguments without writing tedious parsing logic
- You want simple time queries, `sleep`, and scope-based timers that can log on demand and be disabled globally
- The standard library lacks common path utilities like `join` and `exists`; platform-specific APIs aren't portable
- Template error messages are deeply nested — hard to find the real cause
- Bottom line: you don't want heavy third-party libraries that add learning overhead and environment management burden, and you don't want to re-solve the same problems for every project

**How BokuMeidoCpp solves them**
- `format`, `toStr`, `print`: support any standard-library iterable container (including nested ones). Adapt custom types with a simple `operator<<`; unadapted types produce `<ClassName: Address>` instead of a compile error
- Logger: sync/async output to console or file, with a callback mode to forward all library logs to any target
- Thread pool, object pool, `main()` argument parser, time utilities — clean interfaces that you'll understand at first glance
- Path utilities: cross-platform `join`, `exists`, etc., avoiding platform-specific APIs
- Template type checking: compile errors appear at the call site, not buried deep inside template bodies
- Header-only, zero external dependencies — drop `bokumeido/` into your project, `#include "bokumeido/core.hpp"`, and you're done

---

## Table of Contents

- [Quick Start](#quick-start)
- [Modules](#modules)
- [Quick Benchmarks](#quick-benchmarks)
- [Design Philosophy](#design-philosophy)
- [Compatibility](#compatibility)
- [Notes & Caveats](#notes--caveats)
- [Examples](#examples)
- [Version](#version)
- [Tested Platforms](#tested-platforms)
- [Changelog](#changelog)

---

## Quick Start

Header-only and zero-dependency: drop the `bokumeido/` folder into your project, and a single `#include` unlocks everything.

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

Build and run — no third-party libraries to link:

```bash
g++ -std=c++11 -O2 main.cpp -o app && ./app    # GCC / Clang
cl /std:c++14 /EHsc /O2 main.cpp              # MSVC
```

---

## Modules

| Folder | Module | Description | Namespace |
|:------:|:----:|:---------|:--------:|
| **--** | **core.hpp** | One-click import of all core modules | `meido` |
| `core` | **base.hpp** | Version info & basic macros | `meido::base` |
| `core` | **type.hpp** | Compile-time type checks & constraints | `meido::type` |
| `core` | **str.hpp** | String operations: toStr, format, split | `meido::str` |
| `core` | **log.hpp** | Logging: sync/async file, console | `meido::log` |
| `core` | **math.hpp** | Math utilities (rectangles, etc.) | `meido::math` |
| `core` | **datastruct.hpp** | Custom data structures (circular queue) | `meido::ds` |
| `core` | **mem.hpp** | Memory: object pool, avoiding new/delete | `meido::mem` |
| `core` | **path.hpp** | Path operations: exists, listDir, join | `meido::path` |
| `core` | **thread.hpp** | Threading: pool, spinlock, read-write lock | `meido::thrd` |
| `core` | **time.hpp** | Time: timers, sleep, timestamps | `meido::time` |
| `core` | **io.hpp** | I/O: print, argument parsing, file ops | `meido::io` |

### Bundled Implementations

| File | Source | Author |
|:-----|:-------|:-------|
| `3rdparty/dragonbox.hpp` | [Dragonbox](https://github.com/jk-jeon/dragonbox) | Junekey Jeon |


### Usage

* Import individual modules:

```cpp
#include "bokumeido/core/time.hpp"   // time utilities
#include "bokumeido/core/io.hpp"     // I/O utilities
```

* Or import everything at once:

```cpp
#include "bokumeido/core.hpp"        // all core modules
```

* The version string is defined in `base.hpp`. On Linux, run `strings <binary> | grep version` to inspect the linked BokuMeidoCpp version.

---

## Quick Benchmarks

Rough conclusions from simple test scenarios — for reference only. Actual performance depends on compiler, hardware, and system load. Full data in [`docs/benchmark-report.md`](docs/benchmark-report.md).

**Test platforms**:
- x86_64: Linux · Intel Core i7-8700 · GCC 11.4.0 · C++14*
- ARM64: Linux · RK3588 (Cortex-A76/A55) · GCC 11.3.0 (Buildroot) · C++14*

> *Comparison tests depend on `{fmt}` and `spdlog`, which require `C++14`. The library itself is implemented in `C++11`.

### `str::format`

- Overall in the same ballpark as `{fmt}`, with trade-offs: float and long-string scenarios slightly ahead on GCC-based test platforms (thanks to the Dragonbox algorithm); integer scenarios slightly behind (≤1.5×) on some platforms; consistently faster than `ostringstream` (≈2-5×) in the tested scenarios
- **More convenient by design**: adapts to any user-defined type via `operator<<` — no need to specialize `fmt::formatter<T>`

### `log`

- Trade-offs vs `spdlog`: async enqueue path slightly faster on the tested platforms (≈1-5×); sync/callback path slightly slower (≈1-4×, roughly on par on ARM for sync mode); background disk writes roughly on par — same ballpark overall
- Supports **callback mode** to forward log messages to third-party libraries

### `str::split`

- ~2-6× faster than `Boost.StringAlgorithm` in the tested scenarios; on par with or slightly better than hand-written `find`+`substr`


## Design Philosophy

### Principles

| Principle | Description |
|:-----|:------|
| **Cross-Platform** | Based purely on C/C++ standard library |
| **Type-Safe** | Strict template type checking; compilation errors at call sites |
| **Lightweight & Restrained** | Clean, restrained interfaces; focuses on high-frequency, tedious, or standard-library-missing features |
| **Performance-Oriented** | Optimized wherever possible without complicating the API |
| **Thread-Safe** | All free functions are thread-safe; member functions are thread-safe only where documented |
| **Exception-Averse** | Avoids throwing exceptions except for resource exhaustion |

### Naming Conventions

| Category | Convention | Example |
|:-----|:---------|:-----|
| **Types** | PascalCase | `class ThreadPool {}` |
| **Functions** | camelCase | `void splitString() {}` |
| **Variables** | snake_case | `int task_count;` |
| **Member Variables** | snake_case + trailing `_` | `int ThreadPool::task_count_ = 0;` |
| **Enums** | UPPER_SNAKE_CASE | `enum class Level { INFO, DEBUG }` |
| **Public Macros** | UPPER_SNAKE_CASE + `MEIDO_` prefix | `#define MEIDO_INFO(msg)` |
| **Include Guards** | UPPER_SNAKE_CASE + `_HPP_BOKUMEIDOCPP` suffix | `#define CORE_HPP_BOKUMEIDOCPP` |

### Versioning Policy

> **Major.Minor.Patch**: major for breaking API changes, minor for new features, patch for fixes and optimizations. Deprecated interfaces within the same major version are marked but not removed.

---

## Compatibility

> APIs are not removed or renamed within a major version, but the following changes may still affect compatibility:

| Type | Description | Example |
|:-----|:------|:------|
| **Parameter type changes** | Parameter types may be narrowed or new defaulted parameters added | `void func(int a)` → `void func(short a)` |
| **New overloads** | New overloads may introduce ambiguity | Adding `void func(double)` may make `func(42)` ambiguous |
| **Memory layout** | As a header-only library, class/struct layout may change between versions | ⚠️ Do not rely on `sizeof` or binary serialization |

---

## Notes & Caveats

### Critical Restrictions

| Restriction | Description |
|:-----|:------|
| **🚫 Do not call during static-storage destructors** | Destruction order during program exit is undefined; the library internally depends on static-storage objects |
| **🚫 Do not depend on memory layout** | Layout may change at any time; do not use `sizeof`, `memcpy`, or binary serialization on library types |
| **🚫 Do not use underscore-prefixed namespaces/macros** | Names like `_priv` are internal and subject to change without notice |
| **🚫 Do not use as an ABI boundary** | Header-only libraries are inherently unsuitable as cross-DLL/SO interfaces |

### Platform-Specific Notes

| Platform | Note |
|:---------|:---------|
| **QNX710 (g++ 8.3.0)** | Requires `-std=gnu++11` |
| **Thread Safety Scope** | Thread-safe types only guarantee safety among ordinary member functions, not during construction/destruction |
| **Compiler Warnings** | Unused-parameter warnings are expected and harmless |

---

## Examples

### `io.hpp` — Argument parsing & `print`

```cpp
#include "bokumeido/core/io.hpp"

// ---- ArgumentParser (command-line argument parsing) ----
int main(int argc, char* argv[])
{
    meido::io::ArgumentParser parser;

    // Options defined inline in the parse call
    int ret = parser.parse(argc, argv, {
        {"-v", "--verbose", "show detailed output"},
        {"-h", "--help",    "show this help"},
    }, {
        {"-o", "--output", "output path", "./out"},
        {"-l", "--level",  "log level",   "info"},
    });

    if (ret < 0) return 1;
    if (parser.getBoolOpt("--help")) {
        parser.logPreset();  // prints help message automatically
        return 0;
    }

    bool verbose = parser.getBoolOpt("--verbose");
    std::string out_path = parser.getValueOpt("--output");
    // Usage: ./app -v -o /tmp/out --level debug
    return 0;
}

// ---- print (debug output) ----
void print_example() {
    std::vector<int> vec = {1, 2, 3};
    std::map<int, std::string> mp = {{1, "a"}, {2, "b"}};

    meido::io::print(42, 3.14, "hello", vec, mp);
    // Output: 42 3.14 hello {1, 2, 3} {1:a, 2:b}
    // Unsupported types (no operator<<): <ClassName: 0x7fff...>
    // Thread-safety: each print call is internally locked — concurrent calls never race;
    // output from different threads may interleave between calls (same as mixing std::cout), which is normal
}
```

### `log.hpp` — Logging

```cpp
#include "bokumeido/core/log.hpp"

// Setup (call once before logging)
meido::log::setupSyncLogger(meido::log::Level::DEBUG, "/var/log/app", "myapp");
// Or async: meido::log::setupAsyncLogger(...)

// Log messages
MEIDO_INFO("server started on port {}", 8080);
MEIDO_WARN("disk usage: {}%", 85);
MEIDO_ERROR("failed to connect: {}", err_msg);

// Reset to default config before switching mode
meido::log::resetLogger();

// Callback mode — forward to third-party logger
meido::log::setupSyncLogger(meido::log::Level::INFO, [](meido::log::Message msg) {
    third_party_log(msg.level, msg.msg);
});
MEIDO_INFO("this will be forwarded");  // goes to callback, not to file
```

### `str.hpp` — String conversion and formatting

```cpp
#include "bokumeido/core/str.hpp"

// Any type → string (containers, pairs, tuples, etc.)
auto s1 = meido::str::toStr(std::vector<int>{1, -2, 3});   // "{1, -2, 3}"
auto s2 = meido::str::toStr(std::map<int,std::string>{{1,"a"},{2,"b"}});
// "{1:a, 2:b}"

// Float precision control
auto s3 = meido::str::toStr<2>(3.14159);   // "3.14"

// format: use {} as placeholders
auto s4 = meido::str::format("{} × {} = {}", 6, 7, 6 * 7);  // "6 × 7 = 42"

// Custom types: just overload operator<<
struct Point { int x, y; };
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}
auto s5 = meido::str::toStr(Point{3, 4});  // "(3, 4)"
```

### `time.hpp` — Timing and statistics

```cpp
#include "bokumeido/core/time.hpp"

// Scoped timer — logs on scope exit
{
    meido::time::TimeCounterScope<meido::time::MSEC> guard("process frame");
    process();  
    // output: process frame: 23 ms
}

// Running average — logs every N iterations
meido::time::MeanTimeCounter tc(10);
while (true) {
    tc.markStart("infer");
    infer();
    tc.markEnd("infer");
    tc.logOnWindowReached<meido::time::MSEC>();
    // every 10th iteration: infer: avg 45 ms, total 450 ms
}

// One-shot timing
auto t0 = meido::time::nowSteady();
do_work();
auto cost = meido::time::nowSteady().since<meido::time::MSEC>(t0);
```

### `thread.hpp` — Thread pool

```cpp
#include "bokumeido/core/thread.hpp"

int compute(int x) { return x * x; }

int main() {
    meido::thrd::ThreadPool pool(4);  // 4 worker threads

    // Submit tasks (addTask takes a nullary callable), get futures
    auto fut1 = pool.addTask([] { return compute(7); });
    auto fut2 = pool.addTask([] { return compute(9); });

    // Wait and get results (get returns pointer)
    fut1.wait();
    const int* r1 = fut1.get();   // *r1 == 49

    const int* r2 = fut2.get();   // *r2 == 81

    // Fire-and-forget — call wait() so the task completes before pool destruction
    auto fut3 = pool.addTask([] { /* do something */ });
    fut3.wait();

    return 0;
}
```

### `type.hpp` — Template type constraints

```cpp
#include "bokumeido/core/type.hpp"

// T must be one of {int, char, long long}
template <class T, typename std::enable_if<
    meido::type::InTypesChecker<T, int, char, long long>::value, int>::type = 0>
void func(T val) {
    // OK: func(42), func('x'), func(123LL)
    // Error: func(1.5) — error appears at call site, not inside template body
}
```



## Version

| Item | Info |
|:-----|:------|
| **Version** | `1.0.0` |
| **Last Updated** | `2026-08-03` |

---

## Tested Platforms

### Windows

| Item | Info |
|:-----|:------|
| **Compiler** | MSVC 19.50 (VS 2026) |
| **Build Script** | `unit_test/build_msvc.bat` |

### Linux

| Architecture | Compiler | Version | Build Script |
|:-----|:------|:-----|:---------|
| x86_64 | g++ | 11.4.0 | `unit_test/build_gcc.sh` |
| aarch64 (RK3588) | g++ (cross) | 11.3.0 | `unit_test/build_cross.sh` |

---

## Changelog

### v1.0.0

*2026-08-03*

1. Redesigned based on mineutils with standardized interfaces, new feature modules, and performance optimizations;
2. Added unit tests and performance benchmarks.


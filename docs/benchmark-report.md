# BokuMeidoCpp 性能对比测试报告

> **测试日期**：2026-07-31（平台 A/B）· 2026-08-03（平台 C/D）
> **测试库**：BokuMeidoCpp 1.0.0 · {fmt} · spdlog · Boost · BS::thread_pool 5.1.0
> （平台 A/B 为本地源码树 header-only 模式；平台 C/D 使用系统安装的 fmt/spdlog/boost 头文件，同样为 header-only 模式；平台 D 为交叉编译）
>
> **⚠️ 非专业测试声明**：本报告为业余环境下的简单对比测量，非专业基准测试。数据受 CPU 频率调节、系统负载、编译器版本、磁盘类型等环境因素影响，单次采样波动较大，**仅反映测试场景下的相对趋势，不构成性能承诺或结论**，引用请谨慎。

## 测试平台

| 项目 | 平台 A — Windows (MSVC) | 平台 B — WSL2 Ubuntu 22.04 (GCC) | 平台 C — 原生 Linux (GCC) | 平台 D — RK3588 (ARM64) |
|:-----|:------------------------|:----------------------------------|:--------------------------|:--------------------------|
| CPU | AMD Ryzen 5 5600X (6C/12T) | 同左（同一物理机，WSL2 虚拟机） | Intel Core i7-8700 (6C/12T) | RK3588 八核（4×A76 + 4×A55） |
| 编译器 | MSVC 19.50.35730 | GCC 11.4.0 | GCC 11.4.0 | GCC 11.3.0（buildroot 交叉） |
| 优化 | `/O2 /DNDEBUG` | `-O3 -fno-omit-frame-pointer` | 同左 | 同左 |
| C++ 标准 | C++14（线程池对比 C++17） | 同左 | 同左 | 同左 |
| 其他 | — | — | 31GiB 内存 | 2GB 内存、eMMC 存储、Linux 5.10.160 |

## 测试方法

- 时间驱动基准（`bench_common.hpp` v2.0）：每项**预热 200ms** 后采样 **3 轮 × 1s**，取**中位数**并输出 min/max
- **自适应批次**：慢操作自动降为单次批次（`batch=1`），避免整批超时浪费
- 所有对比在**同一进程内**交替采样（A/B/A/B 模式），双方使用相同输入数据与容量参数
- 线程池对比为**端到端语义**（提交 + 等待执行完成），三方对等
- 平台 A/B 第三方依赖经 NTFS junction（`benchmark_cmp/3rdparty` → `D:\Coding\3rdparty`）以 header-only 模式编译，无系统级依赖；平台 C 使用系统安装的 fmt/spdlog/boost 头文件（header-only 模式），编译选项与平台 B 相同；平台 D 为交叉编译（aarch64 GCC 11.3.0），同样 header-only 模式
- 平台 D 内存仅 2GB：内存池模块 `--mem-n` 由 1000 万调小为 **200 万（≈45MB）**（METHODOLOGY 允许小内存机器调小并记录）；日志模块经内存评估（异步队列 ~150MB 峰值）后按默认参数运行，未 OOM

---

## 1. 字符串格式化 — `str::format` vs `{fmt}` vs `ostringstream`

| 测试场景 | BokuMeidoCpp | {fmt} | ostringstream |
|:---------|:---------:|:-----:|:-------------:|
| `int {} {} {}` 纯整数 x3 (MSVC) | 116 ns | **78 ns** | 708 ns |
| `int {} {} {}` 纯整数 x3 (GCC) | **48 ns** | 51 ns | 169 ns |
| `int {} {} {}` 纯整数 x3 (Linux GCC) | 76 ns | **74 ns** | 303 ns |
| `int {} {} {}` 纯整数 x3 (ARM) | 147 ns | **123 ns** | 882 ns |
| `float {} {} {}` 短浮点 x3 (MSVC) | **200 ns** | 246 ns | 1717 ns |
| `float {} {} {}` 短浮点 x3 (GCC) | **110 ns** | 166 ns | 756 ns |
| `float {} {} {}` 短浮点 x3 (Linux GCC) | **193 ns** | 226 ns | 979 ns |
| `float {} {} {}` 短浮点 x3 (ARM) | **370 ns** | 436 ns | 2436 ns |
| `float {} {} {}` 长浮点 x3 (MSVC) | 304 ns | **257 ns** | 1660 ns |
| `float {} {} {}` 长浮点 x3 (GCC) | **139 ns** | 169 ns | 685 ns |
| `float {} {} {}` 长浮点 x3 (Linux GCC) | **214 ns** | 256 ns | 927 ns |
| `float {} {} {}` 长浮点 x3 (ARM) | **477 ns** | 499 ns | 2413 ns |
| `str {} {} {}` 字符串 x3 (MSVC) | 133 ns | **123 ns** | 308 ns |
| `str {} {} {}` 字符串 x3 (GCC) | **64 ns** | 78 ns | 176 ns |
| `str {} {} {}` 字符串 x3 (Linux GCC) | **76 ns** | 135 ns | 250 ns |
| `str {} {} {}` 字符串 x3 (ARM) | 207 ns | **197 ns** | 898 ns |
| `int={} float={} str={}` 混合 (MSVC) | 171 ns | **169 ns** | 985 ns |
| `int={} float={} str={}` 混合 (GCC) | **83 ns** | 102 ns | 400 ns |
| `int={} float={} str={}` 混合 (Linux GCC) | **107 ns** | 139 ns | 506 ns |
| `int={} float={} str={}` 混合 (ARM) | **261 ns** | 282 ns | 1527 ns |
| 长字符串无占位符 (MSVC) | **43 ns** | 102 ns | 369 ns |
| 长字符串无占位符 (GCC) | **17 ns** | 32 ns | 137 ns |
| 长字符串无占位符 (Linux GCC) | **23 ns** | 50 ns | 207 ns |
| 长字符串无占位符 (ARM) | **52 ns** | 99 ns | 807 ns |

**解读**：四个平台整体趋势基本一致——GCC 系（B/C/D）浮点与长字符串场景快于 {fmt}，字符串/混合场景互有胜负；平台 D（ARM）整数场景落后 {fmt} 约 1.2×（与平台 A 类似），可能与编译器代码生成有关；各平台测试场景中均快于 `ostringstream`（约 2~5×，平台 D 约 2.5~6×）。

---

## 2. 日志 — `log` vs `spdlog`

### 模式 A：无 IO（回调 / null_sink）— 纯 dispatch + format

| 测试场景 | BokuMeidoCpp | spdlog |
|:---------|:---------:|:------:|
| INFO 纯消息 (MSVC) | 89 ns | **34 ns** |
| INFO 纯消息 (GCC) | 51 ns | **24 ns** |
| INFO 纯消息 (Linux GCC) | 67 ns | **24 ns** |
| INFO 纯消息 (ARM) | 144 ns | **56 ns** |
| INFO 带参数 int+double (MSVC) | 200 ns | **131 ns** |
| INFO 带参数 int+double (GCC) | 104 ns | **96 ns** |
| INFO 带参数 int+double (Linux GCC) | 146 ns | **127 ns** |
| INFO 带参数 int+double (ARM) | 353 ns | **238 ns** |
| DEBUG 被 min_level 过滤 (MSVC) | **3 ns** | 5 ns |
| DEBUG 被 min_level 过滤 (GCC) | 0 ns † | 0 ns † |
| DEBUG 被 min_level 过滤 (Linux GCC) | 0 ns † | 0 ns † |
| DEBUG 被 min_level 过滤 (ARM) | 0 ns † | 2 ns |

### 模式 B：同步文件输出（写入 `/tmp`，ext4）

| 测试场景 | BokuMeidoCpp | spdlog |
|:---------|:---------:|:------:|
| INFO 纯消息 (MSVC) | 369 ns | **268 ns** |
| INFO 纯消息 (GCC) | 282 ns | **158 ns** |
| INFO 纯消息 (Linux GCC) | 613 ns | **163 ns** |
| INFO 纯消息 (ARM) | 392 ns | **262 ns** |
| INFO 带参数 int+double (MSVC) | 468 ns | **316 ns** |
| INFO 带参数 int+double (GCC) | 318 ns | **217 ns** |
| INFO 带参数 int+double (Linux GCC) | 805 ns | **261 ns** |
| INFO 带参数 int+double (ARM) | 561 ns | **428 ns** |

### 模式 C：异步文件输出 — 前端入队开销

| 测试场景 | BokuMeidoCpp | spdlog |
|:---------|:---------:|:------:|
| INFO 纯消息（入队）(MSVC) | **145 ns** | 391 ns |
| INFO 纯消息（入队）(GCC) | **83 ns** | 458 ns |
| INFO 纯消息（入队）(Linux GCC) | **127 ns** | 461 ns |
| INFO 纯消息（入队）(ARM) | **327 ns** | 705 ns |
| INFO 带参数（入队）(MSVC) | **305 ns** | 345 ns |
| INFO 带参数（入队）(GCC) | **179 ns** | 396 ns |
| INFO 带参数（入队）(Linux GCC) | **287 ns** | 519 ns |
| INFO 带参数（入队）(ARM) | 670 ns | **828 ns** |

### 模式 D：异步后台写入吞吐（50k 条，容量 200k）

| 测试场景 | BokuMeidoCpp | spdlog |
|:---------|:---------:|:------:|
| 前端入队 ns/op (MSVC) | **174** | 199 |
| 前端入队 ns/op (GCC) | **142** | 242 |
| 前端入队 ns/op (Linux GCC) | **166** | 390 |
| 前端入队 ns/op (ARM) | **465** | 3310 |
| 后台落盘 ns/条 (MSVC) | **1255** | 1267 |
| 后台落盘 ns/条 (GCC) | 1214 | **1211** |
| 后台落盘 ns/条 (Linux GCC) | 1210 | **1208** |
| 后台落盘 ns/条 (ARM) | 1415 | **1212** |

**解读**：
- 异步**入队路径** BokuMeidoCpp 测试场景中快于 spdlog（平台 A/B/C 约 1~5×；平台 D 约 1.2~7×）；同步路径与纯 dispatch 慢约 1~4×（平台 D 同步文件模式与 spdlog 基本持平）——互有取舍，整体同一量级
- **后台落盘**：四个平台双方基本持平或接近（约 1.2~1.4µs/条；平台 D 为 tmpfs 内存盘上的重测值）。⚠️ 平台 D 初次测量的 41µs/405µs 为 **tmpfs 被同步模式写满后的超时伪影，已废弃**（详见"已知说明"）
- 平台 C 同步文件模式的绝对数值（~613/805 ns）比平台 A/B 大，可能受 CPU 型号、磁盘等因素影响，仅供参考
- † 被过滤日志为编译器折叠结果（双方对称，0 ns 表示被优化）

---

## 3. 字符串分割 — `split` vs `Boost.StringAlgorithm` vs 手写

| 测试场景 | BokuMeidoCpp | Boost | 手写 find+substr |
|:---------|:---------:|:-----:|:----------------:|
| `split(s, ",")` 10段 (MSVC) | **163 ns** | 661 ns | 488 ns |
| `split(s, ",")` 10段 (GCC) | **149 ns** | 481 ns | 218 ns |
| `split(s, ",")` 10段 (Linux GCC) | **210 ns** | 686 ns | 343 ns |
| `split(s, ",")` 10段 (ARM) | **402 ns** | 1257 ns | 599 ns |
| `split(s, ",")` 100段 (MSVC) | **2065 ns** | 4077 ns | 2757 ns |
| `split(s, ",")` 100段 (GCC) | **1478 ns** | 3023 ns | 1353 ns |
| `split(s, ",")` 100段 (Linux GCC) | **1951 ns** | 3993 ns | 1788 ns |
| `split(s, ",")` 100段 (ARM) | **3471 ns** | 6651 ns | 3039 ns |
| `splitAny(s, " ,\|\t\n")` 10段 (MSVC) | **258 ns** | 705 ns | — |
| `splitAny(s, " ,\|\t\n")` 10段 (GCC) | **133 ns** | 543 ns | — |
| `splitAny(s, " ,\|\t\n")` 10段 (Linux GCC) | **234 ns** | 705 ns | — |
| `splitAny(s, " ,\|\t\n")` 10段 (ARM) | **362 ns** | 1336 ns | — |
| `split(s, "=>")` 多字符 10段 (MSVC) | **207 ns** | 1002 ns | 501 ns |
| `split(s, "=>")` 多字符 10段 (GCC) | **155 ns** | 868 ns | 194 ns |
| `split(s, "=>")` 多字符 10段 (Linux GCC) | **238 ns** | 1007 ns | 338 ns |
| `split(s, "=>")` 多字符 10段 (ARM) | **474 ns** | 1969 ns | 654 ns |
| `split(s, ",", skip_empty)` (MSVC) | **186 ns** | 700 ns | — |
| `split(s, ",", skip_empty)` (GCC) | **170 ns** | 474 ns | — |
| `split(s, ",", skip_empty)` (Linux GCC) | **238 ns** | 898 ns | — |
| `split(s, ",", skip_empty)` (ARM) | **441 ns** | 1382 ns | — |
| `rsplit(s, ".")` 10段 反向 (MSVC) | 253 ns | — | — |
| `rsplit(s, ".")` 10段 反向 (GCC) | 187 ns | — | — |
| `rsplit(s, ".")` 10段 反向 (Linux GCC) | 319 ns | — | — |
| `rsplit(s, ".")` 10段 反向 (ARM) | 664 ns | — | — |

**解读**：四个平台趋势一致——测试场景中快于 Boost 约 2~6×（平台 D 约 2~3×）；100 段场景与手写基本持平（手写 100 段在部分 GCC 平台略快约 9%，平台 D 快约 12%），10 段场景多数快于手写约 25~45%（避免逐段 substr 拷贝）。平台 C/D 各场景绝对数值比平台 B 略大（约 1.2~1.4×），可能与 CPU 型号/频率有关，仅供参考。

---

## 4. 内存池 — `ObjectPool` vs `Boost.Pool` vs `new/delete`

| 测试场景 | BokuMeidoCpp | Boost.Pool | raw new/delete |
|:---------|:---------:|:-----:|:--------------:|
| 大池 construct/destroy 配对 (MSVC) | 19 ns | **2 ns** | 36 ns |
| 大池 construct/destroy 配对 (GCC) | 11 ns | **2 ns** | 12 ns |
| 大池 construct/destroy 配对 (Linux GCC) | 17 ns | **2 ns** | 14 ns |
| 大池 construct/destroy 配对 (ARM) | 30 ns | **4 ns** | 35 ns |
| 小池 64 槽循环 (MSVC) | 20 ns | **2 ns** | 36 ns |
| 小池 64 槽循环 (GCC) | 11 ns | **2 ns** | 12 ns |
| 小池 64 槽循环 (Linux GCC) | 17 ns | **2 ns** | — |
| 小池 64 槽循环 (ARM) | 30 ns | **4 ns** | — |
| 池满 → nullptr (MSVC) | 8 ns | — | — |
| 池满 → nullptr (GCC) | 5 ns | — | — |
| 池满 → nullptr (Linux GCC) | 7 ns | — | — |
| 池满 → nullptr (ARM) | 14 ns | — | — |

**解读**：Boost.Pool 无锁（2~4 ns）快于 ObjectPool（11~30 ns）——**差距主要来自 ObjectPool 的每步 mutex 锁**（`free_list_` 操作持锁），是线程安全的设计取舍，非测量误差。两者均快于 raw new/delete（12~36 ns，平台 D 约 35 ns）。平台 D 的 vector 参考项（emplace_back）因 2GB 内存不足（时间驱动循环使 vector 持续增长）以 `bad_alloc` 中止，未取得数据，不影响主对比项。

---

## 5. 环形队列 — `CircularQueue` vs `Boost.CircularBuffer` vs `std::deque`

| 测试场景 | BokuMeidoCpp | Boost.CB | std::deque |
|:---------|:---------:|:--------:|:----------:|
| enqueue/push_back 不覆盖 (MSVC) | 5 ns | **1 ns** | — |
| enqueue/push_back 不覆盖 (GCC) | 3 ns | 0 ns † | — |
| enqueue/push_back 不覆盖 (Linux GCC) | 10 ns | 0 ns † | — |
| enqueue/push_back 不覆盖 (ARM) | 5 ns | **1 ns** | — |
| enqueue+dequeue 交替 容量64 (MSVC) | 3 ns | **1 ns** | 2 ns |
| enqueue+dequeue 交替 容量64 (GCC) | 3 ns | **1 ns** | 1 ns |
| enqueue+dequeue 交替 容量64 (Linux GCC) | 11 ns | **1 ns** | 1 ns |
| enqueue+dequeue 交替 容量64 (ARM) | 5 ns | **3 ns** | 4 ns |
| 满容量覆盖 (MSVC) | 3 ns | 0 ns † | — |
| 满容量覆盖 (GCC) | 3 ns | **3 ns** | — |
| 满容量覆盖 (Linux GCC) | 11 ns | **2 ns** | — |
| 满容量覆盖 (ARM) | 5 ns | **1 ns** | — |
| 空队列出队 (MSVC) | 0 ns † | 0 ns † | 0 ns † |
| 空队列出队 (GCC) | 0 ns † | 0 ns † | 0 ns † |
| 空队列出队 (Linux GCC) | 0 ns † | 0 ns † | 0 ns † |
| 空队列出队 (ARM) | 0 ns † | 0 ns † | 0 ns † |

**解读**：均为个位数到十几 ns 的纳秒级操作（平台 C 数值略大，约 10~11 ns；平台 D 约 5 ns），绝对差异实际意义有限；BokuMeidoCpp 的 `tryDequeue` 返回状态值（可安全用于空队列），Boost.CB 空 `pop_front` 为 UB 需自行判空——**API 设计上更安全**。† 编译器折叠结果（双方对称）。

---

## 6. 线程池 — `ThreadPool` vs `BS::thread_pool` vs `std::async`

> **端到端语义**（提交 + 等待完成）

| 测试场景 | BokuMeidoCpp | BS::thread_pool | std::async |
|:---------|:---------:|:---------------:|:----------:|
| 空任务提交+等待 (MSVC) | 15.4 µs | **12.9 µs** | 15.8 µs |
| 空任务提交+等待 (GCC) | 37.4 µs | **36.7 µs** | 59.6 µs |
| 空任务提交+等待 (Linux GCC) | 8.9 µs | **8.0 µs** | 24.9 µs |
| 空任务提交+等待 (ARM) | 44.3 µs | **34.0 µs** | 67.1 µs |
| 提交+获取返回值 (MSVC) | 14.4 µs | **13.3 µs** | 15.6 µs |
| 提交+获取返回值 (GCC) | 37.5 µs | **36.4 µs** | 58.8 µs |
| 提交+获取返回值 (Linux GCC) | 9.0 µs | **8.5 µs** | 24.6 µs |
| 提交+获取返回值 (ARM) | 40.4 µs | **33.7 µs** | 70.4 µs |
| 批量 10000 任务+等待 (MSVC) | 8.2 ms | **6.1 ms** | — |
| 批量 10000 任务+等待 (GCC) | 145.1 ms | **134.9 ms** | — |
| 批量 10000 任务+等待 (Linux GCC) | 24.5 ms | **12.9 ms** | — |
| 批量 10000 任务+等待 (ARM) | 154.0 ms | **34.3 ms** | — |
| std::thread 创建+join 参考 (MSVC) | 75.7 µs | — | — |
| std::thread 创建+join 参考 (GCC) | 56.9 µs | — | — |
| std::thread 创建+join 参考 (Linux GCC) | 22.5 µs | — | — |
| std::thread 创建+join 参考 (ARM) | 64.7 µs | — | — |

**解读**：端到端语义下单任务往返 BokuMeidoCpp 与 BS::thread_pool 基本持平（平台 C 差约 6~11%，平台 D 差约 20~30%，平台 A/B 差约 7~16%）；批量场景平台 C 差约 1.9×、平台 D 差约 4.5×（154 ms vs 34 ms），明显大于平台 A/B 的 8~34%——批量差距在小核/低主频平台上有放大迹象，但该场景仅一次完整测量，可能与 CPU 微架构、系统调度或库实现细节有关，有待进一步验证。四个平台测试场景中均快于 std::async（单任务约 1.5~3×）。

---

## 7. 总结与结论

| 模块 | 结论 |
|:-----|:-----|
| **format** | 测试场景中与 {fmt} 互有胜负、整体同一量级（GCC 系多数场景持平或略快；MSVC/ARM 整数等场景落后 ≤1.5×）；快于 ostringstream（约 2~5×） |
| **log 入队/异步** | 异步路径测试场景中快于 spdlog（异步入队场景约 1~5×）；同步/回调路径慢约 1~4×（ARM 上同步模式基本持平）——互有取舍 |
| **log 后台落盘** | 四个平台与 spdlog 基本持平（~1.2~1.4µs/条；ARM 为 tmpfs 重测值） |
| **split** | 测试场景中快于 Boost 约 2~6×，与手写持平或略优 |
| **ObjectPool** | 慢于 Boost.Pool（锁开销，线程安全取舍）；快于 new/delete |
| **CircularQueue** | 与 Boost.CB 同量级；API 设计更安全（空队列可安全出队） |
| **ThreadPool** | 与 BS::thread_pool 基本持平（平台 C/D 批量场景差距偏大，待验证）；快于 std::async |

### 已知说明
- 本报告数据为 **WSL 环境不稳定**下多次分模块采集（MSVC 逐模块运行、GCC 分模块运行），同平台不同运行间的数值波动（如 log 同步模式 MSVC 两次运行 369~444 ns）属于环境噪声，趋势结论不受影响
- 平台 C（原生 Linux）为单次分模块采集（format/split/mem/queue/log 各模块独立运行 + 线程池独立程序），未做多轮重复验证；其 fmt/spdlog/boost 为系统安装版本，版本号与平台 A/B 的本地源码树可能不同，跨平台直接比较绝对数值时需谨慎
- 平台 D（RK3588 ARM64）为 adb 远程分模块采集：内存仅 2GB，内存池 `--mem-n` 调小为 200 万（≈45MB，已记录）；日志模块经内存评估（异步队列峰值约 150MB）按默认参数运行未 OOM；vector 参考项因内存不足 `bad_alloc` 未完成；`[ENV]` 中 CPU 型号显示为未知（交叉编译环境识别限制），CPU 为 RK3588（4×A76 + 4×A55，未锁频）
- 平台 D log 模块**测量事故记录**：初次按默认参数（预热 200ms × 3 轮 × 1s）运行时，同步文件模式在 1 秒时间窗口内以约 200~300MB/s 持续写入，将 /tmp（tmpfs，仅 981MB）**写满**，导致模式 D 写入失败（spdlog 报 `No space left on device`），`wait_file_stable` 轮询 2s/20s 超时，产出 41µs/405µs 每条的**超时伪影数据（已废弃）**；清空 tmpfs 并以 `--warmup-ms 100 --run-ms 300 --rounds 1`（总写入约 380MB）重测后模式 A~D 数据有效，模式 B 因采样窗口较小（1 轮 × 300ms）噪声偏大，仅供参考
- 本报告为**非专业测试**：未锁频、未隔离 CPU、未静默系统负载，绝对数值仅反映本机本时刻状态；报告中的倍数与百分比仅为数据描述，不构成性能优劣结论
- 空队列/被过滤日志等"0 ns"项为编译器折叠结果（双方对称），不构成性能结论
- 数据文件：`benchmark_cmp/tmp/` 下（`r_*.txt`、`linux_*.json`、`arm_*.json` 等原始输出，保留供复核）

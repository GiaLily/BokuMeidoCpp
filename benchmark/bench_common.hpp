/*  benchmark 公共辅助工具 (v2.0)
    特性：
    - 时间驱动：每项测试采样约 run_ms 毫秒（默认 1s）
    - 预热：每项测试先跑 warmup_ms（默认 200ms）丢弃结果，消除冷启动/频率调节噪声
    - 多轮采样取中位数：rounds 轮（默认 3），输出 min/max，对比差异更可信
    - 自适应批次：探测单次操作耗时，慢操作自动降批次，避免整批超时浪费
    - 结果收集 + JSON 导出（--json 路径），供性能回归对比
    - 宏输出走 bench::out()，io/time 模块重定向 std::cout 时不吞结果
    用法: BENCH_N("描述", { 测试代码; })
    用法: BENCH_N_CONSTEXPR("描述", { 测试代码; })  // 标注可能被编译器消除
*/
#pragma once
#ifndef BENCH_COMMON_HPP
#define BENCH_COMMON_HPP

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace bench
{

// 基准测试全局配置（可用命令行参数覆盖）
struct Config
{
    int64_t warmup_ms = 200;     // 每项测试预热时长（0=关闭）
    int64_t run_ms = 1000;       // 每轮采样时长
    int rounds = 3;              // 采样轮数（1-9）
    int64_t batch_size = 10000;  // 默认每批操作数
    int64_t mem_n = 10000000;    // mem 模块 ObjectPool 默认规模
};

inline Config& config()
{
    static Config cfg;
    return cfg;
}

// 单项测试结果
struct Result
{
    std::string module;    // 所属模块名
    std::string desc;      // 测试描述
    double median_ns = 0;  // 中位数 ns/op
    double min_ns = 0;     // 最小 ns/op
    double max_ns = 0;     // 最大 ns/op
    int64_t iters = 0;     // 全部轮次迭代总数
    bool eliminated = false; // 可能被编译器常量折叠
};

// 当前模块名（C++11 兼容：函数内 static 替代 inline 变量）
inline const char*& moduleNamePtr()
{
    static const char* name = "";
    return name;
}
inline void setModule(const char* name) { moduleNamePtr() = name; }

inline std::vector<Result>& results()
{
    static std::vector<Result> res;
    return res;
}

// 收集一条测试结果（C++11 兼容：手动赋值替代聚合初始化）
inline void addResult(const char* module, const std::string& desc,
                      double median_ns, double min_ns, double max_ns,
                      int64_t iters, bool eliminated)
{
    Result r;
    r.module = module;
    r.desc = desc;
    r.median_ns = median_ns;
    r.min_ns = min_ns;
    r.max_ns = max_ns;
    r.iters = iters;
    r.eliminated = eliminated;
    results().push_back(r);
}

// 宏输出通道：默认 cout；io/time 模块重定向 cout 时切到 cerr 避免结果被吞
inline std::ostream*& outPtr()
{
    static std::ostream* p = &std::cout;
    return p;
}
inline std::ostream& out() { return *outPtr(); }
inline void setOut(std::ostream& os) { outPtr() = &os; }

// 丢弃型 streambuf：输出直接丢弃，不累积内存（替代 ostringstream 无限增长）
class NullStreamBuf : public std::streambuf
{
protected:
    int overflow(int c) override { return c; }
};

inline std::streambuf* redirectCout()
{
    static NullStreamBuf null_buf;
    return std::cout.rdbuf(&null_buf);
}
inline void restoreCout(std::streambuf* old_buf)
{
    if (old_buf) std::cout.rdbuf(old_buf);
}

// JSON 字符串转义（描述中含引号/反斜杠/换行）
inline std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        switch (c)
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

// 将全部结果写入 JSON 文件（供回归对比脚本使用）
inline void writeJsonResults(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "w");
    if (!f)
    {
        std::cerr << "[ERROR] 无法写入 " << path << std::endl;
        return;
    }
    const std::vector<Result>& res = results();
    fprintf(f, "{\n  \"results\": [\n");
    for (size_t i = 0; i < res.size(); ++i)
    {
        const Result& r = res[i];
        fprintf(f,
                "    {\"module\":\"%s\",\"desc\":\"%s\",\"median_ns\":%.2f,"
                "\"min_ns\":%.2f,\"max_ns\":%.2f,\"iters\":%lld,\"eliminated\":%s}%s\n",
                jsonEscape(r.module).c_str(), jsonEscape(r.desc).c_str(),
                r.median_ns, r.min_ns, r.max_ns,
                static_cast<long long>(r.iters),
                r.eliminated ? "true" : "false",
                (i + 1 < res.size()) ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

// 打印环境信息：日期/编译器/优化级别/CPU/核心数（保证结果可复现）
inline void printEnv()
{
    std::cout << "[ENV] 构建日期: " << __DATE__ << " " << __TIME__ << std::endl;
#if defined(__clang__)
    std::cout << "[ENV] 编译器: Clang " << __clang_major__ << "." << __clang_minor__
              << "." << __clang_patchlevel__ << std::endl;
#elif defined(__GNUC__)
    std::cout << "[ENV] 编译器: GCC " << __GNUC__ << "." << __GNUC_MINOR__
              << "." << __GNUC_PATCHLEVEL__ << std::endl;
#elif defined(_MSC_VER)
    std::cout << "[ENV] 编译器: MSVC " << _MSC_VER << std::endl;
#else
    std::cout << "[ENV] 编译器: 未知" << std::endl;
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
    std::cout << "[ENV] 构建类型: Debug" << std::endl;
#elif defined(__OPTIMIZE__)
    std::cout << "[ENV] 构建类型: Release (优化开启)" << std::endl;
#else
    std::cout << "[ENV] 构建类型: 未开启优化 (无 __OPTIMIZE__)" << std::endl;
#endif
#if defined(_MSC_VER)
    const char* cpu = std::getenv("PROCESSOR_IDENTIFIER");
    const char* ncpu = std::getenv("NUMBER_OF_PROCESSORS");
    std::cout << "[ENV] CPU: " << (cpu ? cpu : "未知")
              << " (" << (ncpu ? ncpu : "?") << " 逻辑核心)" << std::endl;
#else
    std::string model = "未知";
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (std::strncmp(line, "model name", 10) == 0)
            {
                std::string s = line;
                size_t p = s.find(':');
                if (p != std::string::npos) s = s.substr(p + 2);
                while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
                    s.pop_back();
                model = s;
                break;
            }
        }
        fclose(f);
    }
    std::cout << "[ENV] CPU: " << model << std::endl;
#endif
}

} // namespace bench

// 采样内核宏：desc 描述，eliminated 是否标注常量折叠风险
// 自适应批次：先探测 100 次，平均耗时 >5µs 则批次降为 1
// 预热 warmup_ms 后，采样 rounds 轮（每轮 run_ms），排序取中位数
#define BENCH_N_IMPL(desc, eliminated, ...)                                    \
    do {                                                                       \
        bench::Config& _cfg = bench::config();                                 \
        int64_t _batch = _cfg.batch_size;                                      \
        {                                                                      \
            int _p_count = 0;                                                  \
            auto _p_start = std::chrono::steady_clock::now();                  \
            for (int _p = 0; _p < 100; ++_p) {                                 \
                __VA_ARGS__;                                                   \
                ++_p_count;                                                    \
                if (_p_count % 10 == 0 &&                                      \
                    std::chrono::steady_clock::now() - _p_start >             \
                        std::chrono::milliseconds(20))                         \
                    break;                                                     \
            }                                                                  \
            auto _p_elapsed = std::chrono::duration_cast<                      \
                std::chrono::nanoseconds>(std::chrono::steady_clock::now() -   \
                                          _p_start).count();                   \
            if (_p_count >= 10 && _p_elapsed / _p_count > 5000)                \
                _batch = 1;                                                    \
        }                                                                      \
        int64_t _nsArr[9] = {0};                                               \
        int64_t _totIters = 0;                                                 \
        int _rc = 0;                                                           \
        for (int _r = 0; _r < _cfg.rounds && _r < 9; ++_r) {                   \
            if (_cfg.warmup_ms > 0) {                                          \
                auto _w_deadline = std::chrono::steady_clock::now() +          \
                    std::chrono::milliseconds(_cfg.warmup_ms);                 \
                while (std::chrono::steady_clock::now() < _w_deadline) {       \
                    for (int64_t _i = 0; _i < _batch; ++_i) { __VA_ARGS__; }   \
                }                                                              \
            }                                                                  \
            int64_t _total = 0;                                                \
            auto _start = std::chrono::steady_clock::now();                    \
            auto _deadline = _start + std::chrono::milliseconds(_cfg.run_ms);  \
            while (std::chrono::steady_clock::now() < _deadline) {             \
                for (int64_t _i = 0; _i < _batch; ++_i) { __VA_ARGS__; }       \
                _total += _batch;                                              \
            }                                                                  \
            auto _end = std::chrono::steady_clock::now();                      \
            auto _ns = std::chrono::duration_cast<std::chrono::nanoseconds>(   \
                _end - _start).count();                                        \
            if (_total > 0) {                                                  \
                _nsArr[_rc++] = _ns / _total;                                  \
                _totIters += _total;                                           \
            }                                                                  \
        }                                                                      \
        if (_rc > 0) {                                                         \
            std::sort(_nsArr, _nsArr + _rc);                                   \
            int64_t _med = _nsArr[_rc / 2];                                    \
            bench::out() << "[BENCH] " << (desc) << ": " << _med               \
                         << " ns/op (min=" << _nsArr[0]                        \
                         << ", max=" << _nsArr[_rc - 1]                        \
                         << ", rounds=" << _rc << ", iters=" << _totIters      \
                         << ", batch=" << _batch                               \
                         << (eliminated ? ", 常量折叠风险" : "") << ")"        \
                         << std::endl;                                         \
            bench::addResult(bench::moduleNamePtr(), (desc),                   \
                static_cast<double>(_med), static_cast<double>(_nsArr[0]),     \
                static_cast<double>(_nsArr[_rc - 1]), _totIters,               \
                (eliminated));                                                 \
        }                                                                      \
    } while (0)

// 普通测试
#define BENCH_N(desc, ...) BENCH_N_IMPL(desc, false, __VA_ARGS__)

// 可能被编译器常量折叠/消除的测试（constexpr 函数等），输出加标注
#define BENCH_N_CONSTEXPR(desc, ...) BENCH_N_IMPL(desc, true, __VA_ARGS__)

// 空循环校准：时间驱动，多轮取中位数（约 run_ms/5 每轮）
#define BENCH_CALIBRATE()                                                      \
    do {                                                                       \
        const int64_t _c_batch = 100000;                                       \
        const int _c_rounds = bench::config().rounds;                          \
        int64_t _c_arr[9] = {0};                                               \
        int _c_rc = 0;                                                         \
        for (int _cr = 0; _cr < _c_rounds && _cr < 9; ++_cr) {                 \
            int64_t _c_total = 0;                                              \
            auto _c_start = std::chrono::steady_clock::now();                  \
            auto _c_deadline = _c_start +                                      \
                std::chrono::milliseconds(bench::config().run_ms / 5);         \
            while (std::chrono::steady_clock::now() < _c_deadline) {           \
                for (int64_t _c = 0; _c < _c_batch; ++_c) {                    \
                    volatile int64_t _sink = _c_total;                         \
                    (void)_sink;                                               \
                    ++_c_total;                                                \
                }                                                              \
            }                                                                  \
            auto _c_end = std::chrono::steady_clock::now();                    \
            auto _c_ns = std::chrono::duration_cast<                           \
                std::chrono::nanoseconds>(_c_end - _c_start).count();          \
            if (_c_total > 0) _c_arr[_c_rc++] = _c_ns / _c_total;              \
        }                                                                      \
        if (_c_rc > 0) {                                                       \
            std::sort(_c_arr, _c_arr + _c_rc);                                 \
            bench::out() << "[CALIB] empty loop: " << _c_arr[_c_rc / 2]        \
                         << " ns/op (rounds=" << _c_rc << ")" << std::endl;    \
        }                                                                      \
    } while (0)

// 打印模块分隔标题（同时记录当前模块名，供结果收集）
#define BENCH_MODULE(name)                                                     \
    do {                                                                       \
        bench::setModule(name);                                                \
        BENCH_SEP();                                                           \
        bench::out() << ">>> " << (name) << std::endl;                         \
        BENCH_SEP();                                                           \
    } while (0)

// 打印分隔线
#define BENCH_SEP() \
    bench::out() << "----------------------------------------" << std::endl

#endif // BENCH_COMMON_HPP

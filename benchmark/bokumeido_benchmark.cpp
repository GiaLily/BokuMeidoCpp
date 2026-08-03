/*  BokuMeidoCpp 性能测试主程序
    用法：
        ./bench_run                  # 运行所有模块
        ./bench_run --str            # 只运行 str 模块
        ./bench_run --base --str     # 运行指定模块
        ./bench_run --help           # 显示帮助
*/
#include "bokumeido/core.hpp"
#include "bench_common.hpp"

#include <algorithm>
#include <cstdlib>

#include "core/base_bench.hpp"
#include "core/type_bench.hpp"
#include "core/str_bench.hpp"
#include "core/log_bench.hpp"
#include "core/datastruct_bench.hpp"
#include "core/mem_bench.hpp"
#include "core/math_bench.hpp"
#include "core/path_bench.hpp"
#include "core/thread_bench.hpp"
#include "core/time_bench.hpp"
#include "core/io_bench.hpp"

using namespace meido;

int main(int argc, char* argv[])
{
    base::logVersion("bokumeido_benchmark");

    // 参数解析
    io::ArgumentParser parser;

    std::vector<io::BooleanOption> bool_opts = {
        {"",   "--base",      "base 模块"},
        {"",   "--type",      "type 模块"},
        {"",   "--str",       "str 模块"},
        {"",   "--log",       "log 模块"},
        {"",   "--datastruct","datastruct 模块"},
        {"",   "--mem",       "mem 模块"},
        {"",   "--math",      "math 模块"},
        {"",   "--path",      "path 模块"},
        {"",   "--thread",    "thread 模块"},
        {"",   "--time",      "time 模块"},
        {"",   "--io",        "io 模块"},
        {"-h", "--help",      "显示此帮助信息"},
    };

    std::vector<io::ValueOption> val_opts = {
        {"", "--warmup-ms", "每项测试预热毫秒数 (默认 200, 0=关闭)", "200"},
        {"", "--rounds",    "每项测试采样轮数 (默认 3, 1-9)", "3"},
        {"", "--run-ms",    "每轮采样毫秒数 (默认 1000)", "1000"},
        {"", "--mem-n",     "mem 模块 ObjectPool 规模 (默认 10000000)", "10000000"},
        {"", "--json",      "结果导出 JSON 文件路径 (默认 NONE 不导出)", "NONE"},
    };

    int ret = parser.parse(argc, argv, bool_opts, val_opts);
    if (ret < 0)
    {
        std::cerr << "[ERROR] 参数解析失败" << std::endl;
        return 1;
    }

    if (parser.getBoolOpt("-h") || parser.getBoolOpt("--help"))
    {
        parser.logPreset();
        return 0;
    }

    // 应用配置参数
    bench::config().warmup_ms = std::atoll(parser.getValueOpt("--warmup-ms").c_str());
    bench::config().rounds    = std::max(1, std::min(9, std::atoi(parser.getValueOpt("--rounds").c_str())));
    bench::config().run_ms    = std::max(100LL, std::atoll(parser.getValueOpt("--run-ms").c_str()));
    bench::config().mem_n     = std::max(64LL, std::atoll(parser.getValueOpt("--mem-n").c_str()));

    bench::printEnv();
    BENCH_SEP();

    // 判断运行模式：无参数或没选中任何模块 → 运行全部
    bool run_all = true;
    bool any_selected = parser.getBoolOpt("--base")    || parser.getBoolOpt("--type") ||
                        parser.getBoolOpt("--str")     || parser.getBoolOpt("--log") ||
                        parser.getBoolOpt("--datastruct") || parser.getBoolOpt("--mem") ||
                        parser.getBoolOpt("--math")    || parser.getBoolOpt("--path") ||
                        parser.getBoolOpt("--thread")  || parser.getBoolOpt("--time") ||
                        parser.getBoolOpt("--io");
    if (any_selected)
        run_all = false;

    BENCH_SEP();
    std::cout << "  BokuMeidoCpp Performance Benchmark" << std::endl;
    if (run_all)
        std::cout << "  模式: 全部模块" << std::endl;
    else
        std::cout << "  模式: 指定模块" << std::endl;
    BENCH_SEP();

    if (run_all || parser.getBoolOpt("--base"))      _meidobasebench::check();
    if (run_all || parser.getBoolOpt("--type"))      _meidotypebench::check();
    if (run_all || parser.getBoolOpt("--str"))       _meidostrbench::check();
    if (run_all || parser.getBoolOpt("--log"))       _meidologbench::check();
    if (run_all || parser.getBoolOpt("--datastruct"))_meidodsbench::check();
    if (run_all || parser.getBoolOpt("--mem"))       _meidomembench::check();
    if (run_all || parser.getBoolOpt("--math"))      _meidomathbench::check();
    if (run_all || parser.getBoolOpt("--path"))      _meidopathbench::check();
    if (run_all || parser.getBoolOpt("--thread"))    _meidothrdbench::check();
    if (run_all || parser.getBoolOpt("--time"))      _meidotimebench::check();
    if (run_all || parser.getBoolOpt("--io"))        _meidoiobench::check();

    BENCH_SEP();
    std::cout << "  所有测试完成" << std::endl;
    BENCH_SEP();

    if (parser.getValueOpt("--json") != "NONE")
    {
        bench::writeJsonResults(parser.getValueOpt("--json"));
        std::cout << "  结果已导出: " << parser.getValueOpt("--json") << std::endl;
    }

    return 0;
}

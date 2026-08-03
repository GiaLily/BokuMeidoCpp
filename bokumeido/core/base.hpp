/*  bokumeidocpp库的版本信息及完整实现需要的基本工具  */
#pragma once
#ifndef BASE_HPP_BOKUMEIDOCPP
#define BASE_HPP_BOKUMEIDOCPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
// windows.h 定义了 ERROR 宏 (#define ERROR 0)，会污染 enum class Level { ERROR = 30 }
// 本库使用 C++ 类型安全的枚举，此粗暴宏必须清除
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // 剔除图形、OLE、Shell 等不常用组件
#endif
#ifndef NOMINMAX
#define NOMINMAX    // 禁止 windows.h 定义 min/max 宏
#endif
#ifndef NOGDI
#define NOGDI    // 禁止引入 GDI 头（含 ERROR 宏的定义）
#endif
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


#define BOKUMEIDOCPP_MAJOR "1"                                                                                                     // 主版本号，对应不向下兼容的API或文件改动
#define BOKUMEIDOCPP_MINOR "0"                                                                                                     // 次版本号，对应不影响现有API使用的新功能增加
#define BOKUMEIDOCPP_PATCH "0"                                                                                                     // 修订版本号，对应不改变API的BUG修复或效能优化
#define BOKUMEIDOCPP_DATE "20260803"                                                                                         // 日期版本号，对应文档和注释级别的改动和测试阶段
#define BOKUMEIDOCPP_VERSION BOKUMEIDOCPP_MAJOR "." BOKUMEIDOCPP_MINOR "." BOKUMEIDOCPP_PATCH "-" BOKUMEIDOCPP_DATE    // 完整版本字符串


#if defined(_MSC_VER) && (!defined(__clang__) || defined(__clang_cl__))
// MSVC 或 Clang-cl（MSVC ABI）
#define MEIDO_FUNCSIG __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
// GCC / MinGW / Linux Clang（GCC ABI）
#define MEIDO_FUNCSIG __PRETTY_FUNCTION__
#else
#define MEIDO_FUNCSIG __func__
#endif

// 命名空间::类名::函数名格式的const char*字符串
#define MEIDO_FUNCNAME meido::_priv::splitFuncName(MEIDO_FUNCSIG, __func__)

// 断言宏：失败时打印表达式并终止程序，不受 NDEBUG 影响
#define MEIDO_ASSERT(expr)                            \
    do                                                \
    {                                                 \
        if (!(expr))                                  \
        {                                             \
            _MEIDO_ERROR("Assertion failed: " #expr); \
            fflush(stdout);                           \
            fflush(stderr);                           \
            ::abort();                                \
        }                                             \
    } while (0)


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace base
{
    // 获取bokumeidocpp库的版本
    constexpr const char* getVersion();

    // 打印bokumeidocpp库的版本，project_name为用户可输入的项目名，过长时会被截断
    void logVersion(const char* project_name = nullptr);

    constexpr bool isBigEndian();

    // ScopeGuard —— 作用域守卫，通过 makeScopeGuard 创建，用 auto 接收
    template <class Callable>
    class ScopeGuard
    {
    public:
        template <class Cb = Callable, decltype(std::declval<Cb&>()(), 0) = 0, typename std::enable_if<std::is_move_constructible<Cb>::value && !std::is_reference<Cb>::value, int>::type = 0>
        ScopeGuard(Cb callable);

        // 取消执行
        void dismiss() noexcept;
        // 立即执行，然后重置为无效状态
        void reset();

        // 支持移动构造，禁止拷贝构造，禁止赋值
        ScopeGuard(ScopeGuard&& other) noexcept;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ~ScopeGuard();

    private:
        bool valid_ = false;
        Callable callable_;
    };

    // 创建作用域守卫，在离开作用域时执行 callable
    template <class Callable, decltype(std::declval<Callable&>()(), 0) = 0, typename std::enable_if<std::is_move_constructible<Callable>::value && !std::is_reference<Callable>::value, int>::type = 0>
    inline ScopeGuard<Callable> makeScopeGuard(Callable callable);

}    // namespace base










/*--------------------------------------------内部实现--------------------------------------------*/

#if defined(_MSC_VER) && (!defined(__clang__) || defined(__clang_cl__))
#define _MEIDO_MSVC_LIKE
#elif defined(__GNUC__) || defined(__clang__)
#define _MEIDO_GCC_LIKE
#else
#error "Unknown compiler: please update _MEIDO_MSVC_LIKE/_MEIDO_GCC_LIKE detection in base.hpp for this target"
#endif

#if defined(_WIN32)
#define _MEIDO_WIN32
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define _MEIDO_UNIX
#else
#error "Unknown OS: please update _MEIDO_WIN32/_MEIDO_UNIX detection in base.hpp for this target"
#endif



#define _MEIDO_DEPRECATED_WARNING(msg) _MEIDO_WARN((_priv::getDeprecatedWarningStr() + msg).c_str(), __FILE__, __LINE__)

#define _MEIDO_GCC_MIN_VERSION(major, minor, patch)       \
    (__GNUC__ > (major)                                   \
     || (__GNUC__ == (major) && __GNUC_MINOR__ > (minor)) \
     || (__GNUC__ == (major) && __GNUC_MINOR__ == (minor) && __GNUC_PATCHLEVEL__ >= (patch)))


#if defined(_MEIDO_MSVC_LIKE)
#define _MEIDO_DEPRECATED(msg) __declspec(deprecated(msg))
#define _MEIDO_EXPORT __declspec(dllexport)
#define _MEIDO_HIDDEN
#define _MEIDO_NOINLINE __declspec(noinline)
#define _MEIDO_NOREMOVE static volatile

#elif defined(_MEIDO_GCC_LIKE)
#define _MEIDO_DEPRECATED(msg) __attribute__((deprecated(msg)))
#define _MEIDO_EXPORT __attribute__((visibility("default")))
#define _MEIDO_HIDDEN __attribute__((visibility("hidden")))
#define _MEIDO_NOINLINE __attribute__((noinline))
#define _MEIDO_NOREMOVE __attribute__((used)) static volatile

#else
#define _MEIDO_DEPRECATED(msg)
#define _MEIDO_EXPORT
#define _MEIDO_HIDDEN
#define _MEIDO_NOINLINE
#define _MEIDO_NOREMOVE volatile
#endif

// #if defined(_MEIDO_WIN32)

// #elif defined(_MEIDO_UNIX)





#define _MEIDO_INFO(fmt_chars)                                                       \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::INFO, false,                    \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)
#define _MEIDO_WARN(fmt_chars)                                                       \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::WARN, false,                    \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)
#define _MEIDO_ERROR(fmt_chars)                                                      \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::ERROR, false,                   \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)

#define _MEIDO_INFO_RAW(fmt_chars)                                                   \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::INFO, true,                     \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)
#define _MEIDO_WARN_RAW(fmt_chars)                                                   \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::WARN, true,                     \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)
#define _MEIDO_ERROR_RAW(fmt_chars)                                                  \
    do                                                                               \
    {                                                                                \
        auto _bokumeido_now = std::chrono::system_clock::now();                          \
        meido::_priv::g_base_logger->log(log::Level::ERROR, true,                    \
                                         __FILE__, __LINE__, _bokumeido_now, fmt_chars); \
    } while (0)

namespace log
{
    // 日志级别，RAW与非RAW类型对应级别同级，RAW类型日志格式与用户输入保持一致，无额外头信息
    enum class Level : int
    {
        DEBUG = 0,
        INFO = 10,
        WARN = 20,
        ERROR = 30,
    };

    // 日志消息结构体
    struct Message
    {
        std::chrono::system_clock::time_point sys_tp{};    // 日志触发时间点
        std::chrono::seconds::rep tp_sec{};
        int usec_part;
        Level level;
        bool is_raw;
        const char* filename;
        int line;
        std::thread::id thread_id;
        std::string msg;
    };

    // 日志文件滚动策略
    struct RotationPolicy
    {
        size_t target_file_mb = 100;      // 单个日志文件大小软限制，单位MB，取值范围为1-1024
        size_t target_total_mb = 1024;    // 所有日志文件最大总大小软限制，单位MB，最小为1
        int target_keep_days = 30;        // 日志文件最大保留天数软限制，最小为1
    };

}    // namespace log

namespace _priv
{
    // 整型的绝对值函数，避免负数转换为无符号数时的溢出
    template <class T>
    inline constexpr typename std::make_unsigned<T>::type safeAbs(T x)
    {
        using UT = typename std::make_unsigned<T>::type;
        return x < 0 ? static_cast<UT>(-(x + 1)) + UT(1) : static_cast<UT>(x);
    }

    inline std::string normPath(std::string path)
    {
        if (path.empty())
            return path;

        std::string result;
        result.reserve(path.size());

        const size_t n = path.size();
        size_t i = 0;

        while (i < n)
        {
            char c = path[i];

            // 统一分隔符：\\ → /
            if (c == '\\') c = '/';

            // 跳过 "./" — 在开头或前一个是 / 时
            if (c == '.' && (result.empty() || result.back() == '/'))
            {
                if (i + 1 < n && (path[i + 1] == '/' || path[i + 1] == '\\'))
                {
                    i += 2;    // 跳过 "./" 两个字符
                    continue;
                }
            }

            // 跳过连续的 /
            if (c == '/' && !result.empty() && result.back() == '/')
            {
                ++i;
                continue;
            }

            result += c;
            ++i;
        }

        // 尾部 "/." → 去掉 "."
        if (result.size() >= 2 && result[result.size() - 2] == '/' && result.back() == '.')
            result.pop_back();

        // 尾部 "/" → 去掉（保留 / 和 C:/ 这样的根路径）
        if (result.size() > 1 && result.back() == '/')
        {
            bool is_root = (result.size() == 1);
            bool is_drive_root = (result.size() == 3 && result[1] == ':');
            if (!is_root && !is_drive_root)
                result.pop_back();
        }

        if (result.empty())
            return std::string(".");

        return result;
    }

    inline bool isAbs(const std::string& path)
    {
        return path.find(":/") == 1 || path.substr(0, 1) == "/";
    }

    inline bool isDir(const std::string& path)
    {
        struct stat buffer;
// // MSVC 的 <sys/stat.h> 不提供 POSIX 的 S_ISDIR 宏，手动定义
// #if !defined(S_ISDIR)
// #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
// #endif
#if defined(_MEIDO_WIN32)
        return ::stat(path.c_str(), &buffer) == 0 && ((buffer.st_mode) & S_IFMT) == S_IFDIR;
#else
        return ::stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
#endif
    }

    inline bool exists(const std::string& path)
    {
        struct stat buffer;
        return ::stat(path.c_str(), &buffer) == 0;
    }

    inline bool requireDirs(const std::string& path)
    {
        if (_priv::exists(path))
            return true;
        size_t sep_pos = 0;
        std::string pth_tree;
#if defined(_MEIDO_WIN32)
        if (_priv::isAbs(path))
            sep_pos = 2;
#endif
        do
        {
            sep_pos = path.find("/", sep_pos + 1);
            pth_tree = path.substr(0, sep_pos);
#if defined(_MEIDO_WIN32)
            if (!_priv::isDir(pth_tree) && 0 != ::_mkdir(pth_tree.c_str()))
#else
            if (!_priv::isDir(pth_tree) && 0 != ::mkdir(pth_tree.c_str(), 0777))
#endif
            {
                char err_buf[64];
                const char* err_str = "Unknown error";
#if defined(_MEIDO_MSVC_LIKE)
                if (::strerror_s(err_buf, sizeof(err_buf), errno) == 0)
                    err_str = err_buf;
#elif defined(__GLIBC__) && !defined(_POSIX_C_SOURCE)
                // GNU semantics
                const char* msg = ::strerror_r(errno, err_buf, sizeof(err_buf));
                if (msg)
                    err_str = msg;
#else
                // XSI / POSIX / musl / Apple
                if (::strerror_r(errno, err_buf, sizeof(err_buf)) == 0)
                    err_str = err_buf;
#endif
                printf("[WARN][%s:%d] Failed to call mkdir(%s), %s\n",
                       __FILE__, __LINE__, pth_tree.c_str(), err_str);
            }
        } while (sep_pos != std::string::npos);
        return _priv::isDir(path);
    }

    inline bool fsyncByPath(const std::string& path)
    {
#if defined(_MEIDO_WIN32)
        HANDLE h = ::CreateFileA(
            path.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,    // ✅ 修正
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (h == INVALID_HANDLE_VALUE)
        {
            DWORD err = ::GetLastError();
            return (err == ERROR_FILE_NOT_FOUND);
        }

        BOOL ok = ::FlushFileBuffers(h);
        ::CloseHandle(h);

        return ok != FALSE;
#else
        FILE* fp = fopen(path.c_str(), "rb+");
        if (!fp)
            return false;

        int fd = ::fileno(fp);
        if (fd == -1)
        {
            fclose(fp);
            return false;
        }

        bool ok = (::fsync(fd) == 0);
        fclose(fp);
        return ok;
#endif
    }

    inline size_t getFileSize(const std::string& path)
    {
#if defined(_MEIDO_WIN32)
        WIN32_FILE_ATTRIBUTE_DATA attr;
        if (!::GetFileAttributesExA(
                path.c_str(),
                GetFileExInfoStandard,
                &attr))
            return 0;

        ULARGE_INTEGER ul;
        ul.LowPart = attr.nFileSizeLow;
        ul.HighPart = attr.nFileSizeHigh;
        return ul.QuadPart;

#else
        struct stat st;
        if (::stat(path.c_str(), &st) != 0)
            return 0;
        return static_cast<size_t>(st.st_size);
#endif
    }

    class LogMap
    {
    public:
        LogMap() = default;

        // 假定 rotation_policy 合法
        LogMap(const std::string& log_map_dir, log::RotationPolicy rotation_policy)
        {
            constexpr const char* log_part_ = "/BokuMeidoLogMap.txt";
            log_map_path_tmp_ = log_map_dir + log_part_ + ".tmp";
            log_map_path_bak_ = log_map_dir + log_part_ + ".bak";
            log_map_path_ = log_map_dir + log_part_;
            rotation_policy_ = rotation_policy;
            this->loadLogMap();
        }

        int updateActive(std::string log_path, std::chrono::seconds::rep create_tp_sec)
        {
            auto limit_keep_seconds = rotation_policy_.target_keep_days * 60 * 60 * 24;
            size_t limit_total_size = rotation_policy_.target_total_mb * 1024u * 1024u - rotation_policy_.target_file_mb * 1024u * 1024u;
            while (true)    // 清理过期日志
            {
                if (!log_queue_.empty())
                {
                    const LogInfo& oldest = log_queue_.front();
                    if (create_tp_sec - oldest.create_tp_sec < limit_keep_seconds && total_inactive_size_ < limit_total_size)
                        break;
                    else
                    {
                        remove(oldest.log_path.c_str());
                        total_inactive_size_ -= oldest.size;
                        log_queue_.pop_front();
                    }
                }
                else
                    break;
            }

            if (!log_queue_.empty())
            {
                log_queue_.back().active = false;
                log_queue_.back().size = getFileSize(log_queue_.back().log_path);
                total_inactive_size_ += log_queue_.back().size;
            }
            LogInfo new_info;
            new_info.log_path = std::move(log_path);
            new_info.create_tp_sec = create_tp_sec;
            new_info.active = true;
            log_queue_.emplace_back(std::move(new_info));

            std::ofstream log_map_file(log_map_path_tmp_, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!log_map_file.good())    // 如果这个文件打不开，那么没必要考虑日志保存了，这是系统级问题
            {
                char err_buf[64];
                const char* err_str = "Unknown error";
#if defined(_MEIDO_MSVC_LIKE)
                if (::strerror_s(err_buf, sizeof(err_buf), errno) == 0)
                    err_str = err_buf;
#elif defined(__GLIBC__) && !defined(_POSIX_C_SOURCE)
                // GNU semantics
                const char* msg = ::strerror_r(errno, err_buf, sizeof(err_buf));
                if (msg)
                    err_str = msg;
#else
                // XSI / POSIX / musl / Apple
                if (::strerror_r(errno, err_buf, sizeof(err_buf)) == 0)
                    err_str = err_buf;
#endif
                printf("[ERROR][%s:%d] Failed to open new log map file %s, %s\n",
                       __FILE__, __LINE__, log_map_path_tmp_.c_str(), err_str);
                return -1;
            }

            auto funcEscape = [](const std::string& s) {
                std::string out;
                out.reserve(s.size());
                for (char c : s)
                {
                    if (c == '#')
                        out.append("\\#");
                    else if (c == '\\')
                        out.append("\\\\");
                    else
                        out.append(1, c);
                }
                return out;
            };
            for (auto& info : log_queue_)
            {
                log_map_file << funcEscape(info.log_path) << "##" << info.create_tp_sec << "##" << info.active << std::endl;
            }
            log_map_file.close();
            fsyncByPath(log_map_path_tmp_);

            if (_priv::exists(log_map_path_))    // 如果map存在，那就删除bak map
                remove(log_map_path_bak_.c_str());
            rename(log_map_path_.c_str(), log_map_path_bak_.c_str());
            rename(log_map_path_tmp_.c_str(), log_map_path_.c_str());
            fsyncByPath(log_map_path_);
            remove(log_map_path_bak_.c_str());

            return 0;
        }

        void discardPending()
        {
            if (!log_queue_.empty())
                log_queue_.pop_back();
        }

    private:
        int loadLogMap()
        {
            if (!_priv::exists(log_map_path_) && !_priv::exists(log_map_path_bak_))
            {
                printf("[INFO] No log map file found\n");
                return 0;
            }
            std::ifstream log_map_file(log_map_path_, std::ios::in | std::ios::binary);
            if (!log_map_file.good())
            {
                printf("[WARN][%s:%d] Failed to open log map file %s, try to open backup file\n, %s\n",
                       __FILE__, __LINE__, log_map_path_.c_str(), log_map_path_bak_.c_str());
                log_map_file.close();
                log_map_file.open(log_map_path_bak_, std::ios::in | std::ios::binary);
                if (!log_map_file.good())
                {
                    char err_buf[64];
                    const char* err_str = "Unknown error";
#if defined(_MEIDO_MSVC_LIKE)
                    if (::strerror_s(err_buf, sizeof(err_buf), errno) == 0)
                        err_str = err_buf;
#elif defined(__GLIBC__) && !defined(_POSIX_C_SOURCE)
                    // GNU semantics
                    const char* msg = ::strerror_r(errno, err_buf, sizeof(err_buf));
                    if (msg)
                        err_str = msg;
#else
                    // XSI / POSIX / musl / Apple
                    if (::strerror_r(errno, err_buf, sizeof(err_buf)) == 0)
                        err_str = err_buf;
#endif
                    printf("[WARN][%s:%d] Failed to open backup file %s, %s\n",
                           __FILE__, __LINE__, log_map_path_.c_str(), err_str);
                    return -1;
                }
            }
            else
                remove(log_map_path_bak_.c_str());

            auto funcUnEscape = [](const std::string& s) {
                std::string out;
                out.reserve(s.size());
                for (size_t i = 0; i < s.size(); ++i)
                {
                    if (s[i] == '\\' && i + 1 < s.size())
                    {
                        char next = s[i + 1];
                        if (next == '#' || next == '\\')
                        {
                            out.append(1, next);
                            ++i;
                            continue;
                        }
                    }
                    out += s[i];
                }
                return out;
            };
            std::stringstream ss;
            while (!log_map_file.eof())
            {
                std::string line;    // path##16340224001##0##102400
                std::getline(log_map_file, line);
                if (line.empty())
                    continue;
                LogInfo info;
                size_t path_end_pos = line.find("##");    // 最后一位+1
                if (path_end_pos == std::string::npos)
                {
                    printf("[WARN][%s:%d] Invalid log map line %s, skip.\n", __FILE__, __LINE__, line.c_str());
                    continue;
                }
                info.log_path = funcUnEscape(line.substr(0, path_end_pos));

                size_t time_end_pos = line.find("##", path_end_pos + 2);    // 最后一位+1
                if (time_end_pos == std::string::npos || line.size() != time_end_pos + 3)
                {
                    printf("[WARN][%s:%d] Invalid log map line %s, skip.\n", __FILE__, __LINE__, line.c_str());
                    continue;
                }
                ss.str(std::string());
                ss.clear();
                ss.str(line.substr(path_end_pos + 2, time_end_pos - path_end_pos - 2));
                ss >> info.create_tp_sec;
                if (!ss || !ss.eof())    // 过滤非纯数字的非法时间戳值
                {
                    printf("[WARN][%s:%d] Invalid log map line %s, skip.\n", __FILE__, __LINE__, line.c_str());
                    continue;
                }

                if (line.size() != time_end_pos + 3)    // 过滤长度不为1的active字段
                {
                    printf("[WARN][%s:%d] Invalid log map line %s, skip.\n", __FILE__, __LINE__, line.c_str());
                    continue;
                }
                try
                {
                    info.active = std::stoi(line.substr(time_end_pos + 2, 1));
                }
                catch (...)
                {
                    printf("[WARN][%s:%d] Invalid log map line %s, skip.\n", __FILE__, __LINE__, line.c_str());
                    continue;
                }

                info.size = getFileSize(info.log_path);
                total_inactive_size_ += info.size;
                log_queue_.push_back(std::move(info));
            }
            log_map_file.close();
            return 0;
        }


        struct LogInfo
        {
            std::string log_path;
            std::chrono::seconds::rep create_tp_sec{0};
            bool active{true};

            size_t size{0};
        };

        std::deque<LogInfo> log_queue_;
        std::string log_map_path_;
        std::string log_map_path_tmp_;    // 未完成的log map文件路径
        std::string log_map_path_bak_;    // 旧的log map文件路径

        log::RotationPolicy rotation_policy_;
        size_t total_inactive_size_{0};
    };

    // 两个底层logger只保证init之后其他接口的线程安全，init接口的线程安全和一次性调用由上一层分发器保障
    // init要在其它接口之前也由上层实现
    class SyncLoggerDetail;
    class AsyncLoggerDetail;

    // 该处以后再真正实现析构后的管理吧，C++对于main函数退出后的析构顺序极难控制，暂时先不考虑这种情况
    // void validSyncLog(BaseSyncLogger* sync_logger, log::Level level, const char* filename, int line, std::string msg)
    // {
    //     sync_logger->log(level, filename, line, std::move(msg));
    // }
    // void validAsyncLog(BaseAsyncLogger* async_logger, log::Level level, const char* filename, int line, std::string msg)
    // {
    //     async_logger->log(level, filename, line, std::move(msg));
    // }

    // void emptySyncLog(BaseSyncLogger* sync_logger, log::Level level, const char* filename, int line, std::string msg)
    // {
    // }
    // void emptyAsyncLog(BaseAsyncLogger* async_logger, log::Level level, const char* filename, int line, std::string msg)
    // {
    // }

    // ---- 手动 buffer 拼装辅助函数（替代 fprintf 格式串解析） ----
    inline const char* digits2(size_t value) noexcept
    {
        // alignas(2) 保证 2 字节对齐，避免跨硬件边界慢速访问
        alignas(2) static constexpr char data[] =
            "0001020304050607080910111213141516171819"
            "2021222324252627282930313233343536373839"
            "4041424344454647484950515253545556575859"
            "6061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899";
        return &data[value * 2];
    }

    // 理由：fprintf 每次运行时解析 "%04d-%02d..." 格式串 + va_list 变参展开，
    // 而我们的格式结构在编译期已固定，直接写入即可，省去所有运行时解析开销。
    // 优化：用 digits2 查表替代逐位除法/取余，每次写入2字节
    inline void write2d(char* p, int v)
    {
        const char* d = digits2(static_cast<size_t>(v));
        p[0] = d[0];
        p[1] = d[1];
    }
    inline void write4d(char* p, int v)
    {
        const char* d1 = digits2(static_cast<size_t>(v / 100));
        const char* d2 = digits2(static_cast<size_t>(v % 100));
        p[0] = d1[0];
        p[1] = d1[1];
        p[2] = d2[0];
        p[3] = d2[1];
    }
    inline void write6d(char* p, int v)
    {
        const char* d1 = digits2(static_cast<size_t>(v / 10000));
        const char* d2 = digits2(static_cast<size_t>((v / 100) % 100));
        const char* d3 = digits2(static_cast<size_t>(v % 100));
        p[0] = d1[0];
        p[1] = d1[1];
        p[2] = d2[0];
        p[3] = d2[1];
        p[4] = d3[0];
        p[5] = d3[1];
    }

    inline size_t levelToBuf(log::Level level, char* buf)
    {
        switch (level)
        {
        case log::Level::DEBUG:
            memcpy(buf, "DEBUG", 5);
            return 5;
        case log::Level::INFO:
            memcpy(buf, "INFO", 4);
            return 4;
        case log::Level::WARN:
            memcpy(buf, "WARN", 4);
            return 4;
        case log::Level::ERROR:
            memcpy(buf, "ERROR", 5);
            return 5;
        default:
            memcpy(buf, "UNKNOWN", 7);
            return 7;
        }
    }

    // filename始终地址不变，存储其指针
    inline size_t fileToBuf(const char* filename, char* buf)
    {
        thread_local std::unordered_map<const char*, std::string> cache;
        auto it = cache.find(filename);
        if (it != cache.end())
        {
            memcpy(buf, it->second.c_str(), it->second.size());
            return it->second.size();
        }
        std::string only_filename = normPath(filename);
        only_filename = only_filename.substr(only_filename.rfind('/') + 1);
        auto it2 = cache.emplace(filename, std::move(only_filename)).first;
        memcpy(buf, it2->second.c_str(), it2->second.size());
        return it2->second.size();
    }

    inline size_t lineToBuf(int v, char* buf)
    {
        if (v == 0)
        {
            buf[0] = '0';
            return 1;
        }
        char tmp[16];
        int n = 0;
        while (v > 0)
        {
            tmp[n] = static_cast<char>('0' + v % 10);
            ++n;
            v /= 10;
        }
        for (int i = 0; i < n; ++i)
            buf[i] = tmp[n - 1 - i];
        return n;
    }

    inline size_t tidToBuf(const std::thread::id& id, char* buf)
    {
        thread_local std::unordered_map<std::thread::id, std::string> cache;
        auto it = cache.find(id);
        if (it != cache.end())
        {
            memcpy(buf, it->second.c_str(), it->second.size());
            return it->second.size();
        }

        std::ostringstream oss;
        oss << id;
        auto it2 = cache.emplace(id, oss.str()).first;
        memcpy(buf, it2->second.c_str(), it2->second.size());
        return it2->second.size();
    }

    // 将日志头+消息拼入 buf, 返回总字节数; buf 需 >= 4096
    inline size_t buildLogLine(char* buf, size_t buf_size, const log::Message& log_msg, const tm& local_tm)
    {
        size_t pos = 0;

        // [YYYY-MM-DD HH:MM:SS.uuuuuu]
        buf[pos++] = '[';
        write4d(buf + pos, local_tm.tm_year + 1900);
        pos += 4;
        buf[pos++] = '-';
        write2d(buf + pos, local_tm.tm_mon + 1);
        pos += 2;
        buf[pos++] = '-';
        write2d(buf + pos, local_tm.tm_mday);
        pos += 2;
        buf[pos++] = ' ';
        write2d(buf + pos, local_tm.tm_hour);
        pos += 2;
        buf[pos++] = ':';
        write2d(buf + pos, local_tm.tm_min);
        pos += 2;
        buf[pos++] = ':';
        write2d(buf + pos, local_tm.tm_sec);
        pos += 2;
        buf[pos++] = '.';
        write6d(buf + pos, log_msg.usec_part);
        pos += 6;

        // ][LEVEL]
        buf[pos++] = ']';
        buf[pos++] = '[';
        size_t lv_len = levelToBuf(log_msg.level, buf + pos);
        pos += lv_len;

        // ][file:line]
        buf[pos++] = ']';
        buf[pos++] = '[';
        size_t fn_len = fileToBuf(log_msg.filename, buf + pos);
        pos += fn_len;
        buf[pos++] = ':';
        pos += static_cast<size_t>(lineToBuf(log_msg.line, buf + pos));

        // ][TID:xxx]
        memcpy(buf + pos, "][TID:", 6);
        pos += 6;
        size_t tid_len = tidToBuf(log_msg.thread_id, buf + pos);
        pos += tid_len;

        // 消息（防溢出截断，截断时附加 ... 标记）
        buf[pos++] = ']';
        buf[pos++] = ' ';
        size_t remain = buf_size - pos - 4;    // 预留 "...\n" 的空间
        size_t cpy_size = log_msg.msg.size();
        if (cpy_size > remain)
        {
            memcpy(buf + pos, log_msg.msg.data(), remain);
            pos += remain;
            memcpy(buf + pos, "...\n", 4);
            pos += 4;
        }
        else
        {
            memcpy(buf + pos, log_msg.msg.data(), cpy_size);
            pos += cpy_size;
            buf[pos++] = '\n';
        }

        return pos;
    }


    // 每10秒内复用 localtime 结果，仅通过调整 tm_sec 个位数反映秒变化，避免重复 localtime 系统调用
    inline tm genLocalTm(const log::Message& log_msg)
    {
        thread_local std::chrono::seconds::rep last_sec = 0;
        thread_local tm last_tm = {};

        // 同一精确秒：零开销直接返回
        if (log_msg.tp_sec == last_sec)
            return last_tm;

        auto sec = log_msg.tp_sec;
        // 同一10秒窗口内：直接累加秒差，无需完整 localtime 转换
        if (sec / 10 == last_sec / 10)
        {
            last_tm.tm_sec += static_cast<int>(sec - last_sec);
            last_sec = sec;
            return last_tm;
        }

        time_t t = std::chrono::system_clock::to_time_t(log_msg.sys_tp);
        tm buf{};
#if defined(_MEIDO_WIN32)
        ::localtime_s(&buf, &t);
#else
        ::localtime_r(&t, &buf);
#endif
        last_sec = sec;
        last_tm = buf;
        return buf;
    }

    constexpr size_t g_log_size = 1024;  // 控制一行日志的最大字节数，超过部分截断

    inline void printfLog(const log::Message& log_msg)
    {
        FILE* target = stdout;
        if (log_msg.level >= log::Level::WARN)
            target = stderr;
        if (log_msg.is_raw)
        {
            fwrite(log_msg.msg.data(), 1, log_msg.msg.size(), target);
            fputc('\n', target);
        }
        else
        {
            tm local_tm = genLocalTm(log_msg);
            char buf[g_log_size];
            size_t len = buildLogLine(buf, sizeof(buf), log_msg, local_tm);
            fwrite(buf, 1, len, target);
        }
    }


    class AsyncLoggerDetail
    {
    public:
        AsyncLoggerDetail()
        {}

        int init(std::string log_dir, std::string name_prefix, log::RotationPolicy rotation_policy, size_t capacity)
        {
            if (!_priv::requireDirs(log_dir))
            {
                printf("[ERROR][%s:%d] Failed to create log directory: %s\n", __FILE__, __LINE__, log_dir.c_str());
                return -1;
            }
            else
                printf("[INFO][%s:%d] Log directory: %s\n", __FILE__, __LINE__, log_dir.c_str());

            capacity_.store(capacity, std::memory_order_relaxed);

            log_map_ = LogMap(log_dir, rotation_policy);
            work_thrd_ = std::thread(&AsyncLoggerDetail::workThrd, this, std::move(log_dir), std::move(name_prefix), rotation_policy.target_file_mb * 1024 * 1024);
            return 0;
        }

        void log(log::Message log_msg)
        {
            if (need_abort_.load(std::memory_order_acquire))
                return;
            std::unique_lock<std::mutex> queue_lk(queue_mtx_);
            if (front_queue_.size() >= capacity_.load(std::memory_order_relaxed))
                front_queue_.pop();
            front_queue_.push(std::move(log_msg));
            if (back_waiting_.load(std::memory_order_relaxed))
                queue_cv_.notify_one();
        }

        AsyncLoggerDetail(const AsyncLoggerDetail&) = delete;
        AsyncLoggerDetail& operator=(const AsyncLoggerDetail&) = delete;

        ~AsyncLoggerDetail()
        {
            {    // 避免条件变量消息通知被忽略
                std::lock_guard<std::mutex> queue_lk(queue_mtx_);
                need_abort_.store(true, std::memory_order_release);
            }
            queue_cv_.notify_one();
            if (work_thrd_.joinable())
                work_thrd_.join();
        }

    private:
        void workThrd(std::string log_dir, std::string name_prefix, size_t target_log_size)
        {
            FILE* log_file = nullptr;
            std::string log_file_path;
            int year = -1;
            int month = -1;
            int day = -1;
            size_t file_size = 0;

            while (!need_abort_.load(std::memory_order_relaxed))
            {
                log::Message log_msg;

                if (back_queue_.empty())
                {
                    std::unique_lock<std::mutex> queue_lk(queue_mtx_);
                    // 前台只会置非空，所以此处检查为非空不需要第二次检查了
                    if (!front_queue_.empty())
                    {
                        std::swap(front_queue_, back_queue_);
                        continue;    // swap后跳出本次循环从头开始
                    }
                    while (front_queue_.empty() && !need_abort_.load(std::memory_order_relaxed))
                    {
                        back_waiting_.store(true, std::memory_order_relaxed);
                        queue_cv_.wait(queue_lk);
                    }
                    back_waiting_.store(false, std::memory_order_relaxed);
                    if (need_abort_.load(std::memory_order_relaxed))
                        break;    // 退出时，写完所有日志再结束
                    continue;
                }

                log_msg = std::move(back_queue_.front());
                back_queue_.pop();

                tm local_tm = genLocalTm(log_msg);

                // 如果日志文件大小超过目标大小，或者日期发生变化，关闭当前日志文件，打开新的日志文件
                if (!log_file || file_size >= target_log_size
                    || year != local_tm.tm_year + 1900 || month != local_tm.tm_mon + 1 || day != local_tm.tm_mday)
                {
                    year = local_tm.tm_year + 1900;
                    month = local_tm.tm_mon + 1;
                    day = local_tm.tm_mday;

                    if (log_file)
                    {
                        fclose(log_file);
                        fsyncByPath(log_file_path.c_str());
                    }
                    char tail[64];
                    snprintf(tail, sizeof(tail), "_%04d%02d%02d_%02d%02d%02d.log",
                             year, month, day, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

                    log_file_path = log_dir + name_prefix + tail;
                    log_map_.updateActive(log_file_path, log_msg.tp_sec);
                    log_file = fopen(log_file_path.c_str(), "wb");
                    file_size = 0;
                    if (!log_file)
                    {
                        printf("[ERROR][%s:%d] Failed to create log file: %s\n", __FILE__, __LINE__, log_file_path.c_str());
                        log_map_.discardPending();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    else
                        printf("[INFO][%s:%d] Created log file: %s\n", __FILE__, __LINE__, log_file_path.c_str());
                }
                if (log_msg.is_raw)
                {
                    fwrite(log_msg.msg.data(), 1, log_msg.msg.size(), log_file);
                    fputc('\n', log_file);
                    file_size += log_msg.msg.size() + 1;
                }
                else
                {
                    char buf[g_log_size];
                    size_t len = buildLogLine(buf, sizeof(buf), log_msg, local_tm);
                    file_size += fwrite(buf, 1, len, log_file);
                }
                if (log_msg.level >= log::Level::WARN)
                    fflush(log_file);
            }
            if (log_file)
            {
                fclose(log_file);
                fsyncByPath(log_file_path.c_str());
            }
        }

        // std::atomic<void (*)(BaseAsyncLogger*, log::Level, const char*, int, std::string)>* call_func_ = nullptr;

        std::atomic<bool> need_abort_{false};
        std::thread work_thrd_;
        std::atomic<size_t> capacity_{1024};

        std::mutex queue_mtx_;
        std::condition_variable queue_cv_;
        std::atomic<bool> back_waiting_{false};
        std::queue<log::Message> front_queue_;
        std::queue<log::Message> back_queue_;

        LogMap log_map_;
    };

    class SyncLoggerDetail
    {
    public:
        SyncLoggerDetail() = default;

        int init(std::string log_dir, std::string name_prefix, log::RotationPolicy rotation_policy)
        {
            if (!_priv::requireDirs(log_dir))
            {
                printf("[ERROR][%s:%d] Failed to create log directory: %s\n", __FILE__, __LINE__, log_dir.c_str());
                return -1;
            }
            else
                printf("[INFO][%s:%d] Log directory: %s\n", __FILE__, __LINE__, log_dir.c_str());

            log_dir_ = std::move(log_dir);
            name_prefix_ = std::move(name_prefix);
            log_map_ = LogMap(log_dir_, rotation_policy);
            target_log_size_ = rotation_policy.target_file_mb * 1024 * 1024;
            user_func_ = nullptr;    // 日志文件模式下，不使用用户回调函数
            return 0;
        }

        int init(void (*user_func)(log::Message))
        {
            user_func_ = user_func;
            return 0;
        }

        // 上层的原子变量保障了init里的name_prefix_和log_dir_等在调用时已被初始化
        void log(log::Message log_msg)
        {
            if (user_func_)
            {
                user_func_(std::move(log_msg));
                return;
            }

            tm local_tm = genLocalTm(log_msg);
            // 在文件模式下，如果日期发生变化，关闭当前日志文件，打开新的日志文件
            {
                std::unique_lock<std::mutex> lk(file_mtx_);
                if (!log_file_ || file_size_ >= target_log_size_
                    || year_ != local_tm.tm_year + 1900 || month_ != local_tm.tm_mon + 1 || day_ != local_tm.tm_mday)
                {
                    year_ = local_tm.tm_year + 1900;
                    month_ = local_tm.tm_mon + 1;
                    day_ = local_tm.tm_mday;

                    if (log_file_)
                    {
                        fclose(log_file_);
                        fsyncByPath(log_file_path_.c_str());
                    }
                    char tail[64];
                    snprintf(tail, sizeof(tail), "_%04d%02d%02d_%02d%02d%02d.log",
                             year_, month_, day_, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

                    log_file_path_ = log_dir_ + name_prefix_ + tail;
                    log_map_.updateActive(log_file_path_, log_msg.tp_sec);
                    log_file_ = fopen(log_file_path_.c_str(), "wb");
                    file_size_ = 0;
                    if (!log_file_)
                    {
                        printf("[ERROR][%s:%d] Failed to create log file: %s\n", __FILE__, __LINE__, log_file_path_.c_str());
                        log_map_.discardPending();
                        lk.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        return;
                    }
                }

                if (log_msg.is_raw)
                {
                    fwrite(log_msg.msg.data(), 1, log_msg.msg.size(), log_file_);
                    fputc('\n', log_file_);
                    file_size_ += log_msg.msg.size() + 1;
                }
                else
                {
                    char buf[g_log_size];
                    size_t len = buildLogLine(buf, sizeof(buf), log_msg, local_tm);
                    file_size_ += fwrite(buf, 1, len, log_file_);
                }
            }
            if (log_msg.level >= log::Level::WARN)
                fflush(log_file_);
        }

        ~SyncLoggerDetail()
        {
            std::lock_guard<std::mutex> file_lk(file_mtx_);
            if (log_file_)
                fclose(log_file_);
        }

    private:
        // std::atomic<void (*)(BaseSyncLogger*, log::Level, const char*, int, std::string)>* call_func_ = nullptr;
        std::string log_dir_;
        std::string name_prefix_;
        void (*user_func_)(log::Message msg) = nullptr;

        std::mutex file_mtx_;    // 用于保护日志文件的互斥锁
        FILE* log_file_ = nullptr;
        std::string log_file_path_;
        size_t file_size_ = 0;
        size_t target_log_size_ = 0;
        int year_ = -1;
        int month_ = -1;
        int day_ = -1;

        LogMap log_map_;
    };

    // 基础日志，用于库内部日志输出，分发底层同步异步日志器
    // 该层检查参数的语义合法性，保障对外接口的线程安全
    class BaseLogger
    {
    private:
    public:
        BaseLogger()
        {}

        int setupConsole(log::Level min_level)
        {
            std::lock_guard<std::mutex> lk(init_mtx_);
            if (logger_mode_.load(std::memory_order_acquire) != LoggerMode::DEFAULT)
            {
                printf("[WARN][%s:%d] Logger has been initialized, ignore initConsole call\n", __FILE__, __LINE__);
                return 1;
            }
            if (!this->checkValid(min_level, "NOOP", "NOOP", log::RotationPolicy{}, 1024, [](log::Message msg) {}))
                return -1;
            min_level_ = min_level;
            logger_mode_.store(LoggerMode::DEFAULT, std::memory_order_release);    // 保证min_level_在其他线程可见
            return 0;
        }


        int setupSync(log::Level min_level, std::string log_dir, std::string name_prefix, log::RotationPolicy rotation_policy)
        {
            std::lock_guard<std::mutex> lk(init_mtx_);
            if (logger_mode_.load(std::memory_order_acquire) != LoggerMode::DEFAULT)
            {
                printf("[WARN][%s:%d] Logger has been initialized, ignore initSync call\n", __FILE__, __LINE__);
                return 1;
            }

            if (!this->checkValid(min_level, log_dir, name_prefix, rotation_policy, 1024, [](log::Message msg) {}))
                return -1;
            log_dir = _priv::normPath(std::move(log_dir));
            log_dir.append("/");

            sync_logger_ = std::unique_ptr<SyncLoggerDetail>(new SyncLoggerDetail());
            int ret = sync_logger_->init(std::move(log_dir), std::move(name_prefix), rotation_policy);
            if (ret != 0)
                return ret;
            min_level_ = min_level;
            logger_mode_.store(LoggerMode::SYNC, std::memory_order_release);    // 保证min_level_在其他线程可见
            return ret;
        }

        int setupSync(log::Level min_level, void (*user_func)(log::Message))
        {
            std::lock_guard<std::mutex> lk(init_mtx_);
            if (logger_mode_.load(std::memory_order_acquire) != LoggerMode::DEFAULT)
            {
                printf("[WARN][%s:%d] Logger has been initialized, ignore initSync call\n", __FILE__, __LINE__);
                return 1;
            }
            if (!this->checkValid(min_level, "NOOP", "NOOP", log::RotationPolicy{}, 1024, user_func))
                return -1;

            sync_logger_ = std::unique_ptr<SyncLoggerDetail>(new SyncLoggerDetail());
            int ret = sync_logger_->init(user_func);
            if (ret != 0)
                return ret;
            min_level_ = min_level;
            logger_mode_.store(LoggerMode::SYNC, std::memory_order_release);    // 保证初始化改动在其他线程可见
            return ret;
        }

        int setupAsync(log::Level min_level, std::string log_dir, std::string name_prefix, log::RotationPolicy rotation_policy, size_t capacity)
        {
            std::lock_guard<std::mutex> lk(init_mtx_);
            if (logger_mode_.load(std::memory_order_acquire) != LoggerMode::DEFAULT)
            {
                printf("[WARN][%s:%d] Logger has been initialized, ignore initSync call\n", __FILE__, __LINE__);
                return 1;
            }
            if (!this->checkValid(min_level, log_dir, name_prefix, rotation_policy, capacity, [](log::Message msg) {}))
                return -1;
            log_dir = _priv::normPath(std::move(log_dir));
            log_dir.append("/");

            async_logger_ = std::unique_ptr<AsyncLoggerDetail>(new AsyncLoggerDetail());
            int ret = async_logger_->init(std::move(log_dir), std::move(name_prefix), rotation_policy, capacity);
            if (ret != 0)
                return ret;
            min_level_ = min_level;
            logger_mode_.store(LoggerMode::ASYNC, std::memory_order_release);    // 保证初始化改动在其他线程可见
            return ret;
        }

        void log(log::Level level, bool is_raw, const char* filename, int line, std::chrono::system_clock::time_point now_t, std::string msg)
        {
            if (level < min_level_)    // 确保_MEIDO_LOG路径能够正确过滤
                return;
            auto logger_mode = logger_mode_.load(std::memory_order_acquire);    // 确保初始化改动在当前线程可见

            log::Message log_msg;
            log_msg.sys_tp = now_t;

            log_msg.tp_sec = std::chrono::duration_cast<std::chrono::seconds>(now_t.time_since_epoch()).count();
            log_msg.usec_part = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(now_t.time_since_epoch()).count() % 1000);
            log_msg.level = level;
            log_msg.is_raw = is_raw;
            log_msg.filename = filename;
            log_msg.line = line;
            log_msg.msg = std::move(msg);
            log_msg.thread_id = std::this_thread::get_id();

            if (logger_mode == LoggerMode::DEFAULT)
            {
                printfLog(std::move(log_msg));    // 如果初始化未完成，走单独通道避免使用底层日志器
                return;
            }

            log_call_count_.fetch_add(1, std::memory_order_release);
            if (logger_mode == LoggerMode::SYNC)
                sync_logger_->log(std::move(log_msg));
            else
                async_logger_->log(std::move(log_msg));
            log_call_count_.fetch_sub(1, std::memory_order_release);
        }

        void reset()
        {
            std::lock_guard<std::mutex> lk(init_mtx_);
            logger_mode_.store(LoggerMode::DEFAULT, std::memory_order_release);

            int max_interval_ms = 32;
            int current_interval_ms = 1;
            while (log_call_count_.load(std::memory_order_acquire) > 0)
            {
                // 先简单的等待策略，以后再考虑更复杂的策略
                std::this_thread::sleep_for(std::chrono::milliseconds(current_interval_ms));
                current_interval_ms = std::min(max_interval_ms, current_interval_ms * 2);
            }
            if (sync_logger_)
                sync_logger_.reset();
            if (async_logger_)
                async_logger_.reset();
        }

        // 获取当前最低日志级别，供调用方在格式化前做早期过滤
        log::Level getMinLevel() const
        { return min_level_; }

        BaseLogger(const BaseLogger&) = delete;
        BaseLogger& operator=(const BaseLogger&) = delete;

        ~BaseLogger() {}

    private:
        bool checkValid(log::Level min_level, const std::string& logDir, const std::string& name_prefix,
                        log::RotationPolicy rotation_policy, size_t capacity, void (*user_func)(log::Message))
        {
            switch (min_level)
            {
            case log::Level::ERROR:
            case log::Level::WARN:
            case log::Level::INFO:
            case log::Level::DEBUG:
                break;
            default:
                printf("[ERROR][%s:%d] Invalid min level is %d\n", __FILE__, __LINE__, static_cast<int>(min_level));
                return false;
            }
            if (logDir.empty())
            {
                printf("[ERROR][%s:%d] Log directory is empty\n", __FILE__, __LINE__);
                return false;
            }
            if (name_prefix.empty())
            {
                printf("[ERROR][%s:%d] Log name is empty\n", __FILE__, __LINE__);
                return false;
            }
            if (rotation_policy.target_file_mb == 0 || rotation_policy.target_total_mb == 0
                || rotation_policy.target_keep_days <= 0 || rotation_policy.target_file_mb > rotation_policy.target_total_mb)
            {
                printf("[ERROR][%s:%d] Invalid rotation policy, max_file_mb is %zu, max_total_mb is %zu, max_keep_days is %d\n",
                       __FILE__, __LINE__, rotation_policy.target_file_mb, rotation_policy.target_total_mb, rotation_policy.target_keep_days);
                return false;
            }
            if (capacity == 0)
            {
                printf("[ERROR][%s:%d] Invalid capacity is %zu\n", __FILE__, __LINE__, capacity);
                return false;
            }
            if (!user_func)
            {
                printf("[ERROR][%s:%d] User function is null\n", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        enum class LoggerMode
        {
            DEFAULT,
            SYNC,
            ASYNC
        };
        std::mutex init_mtx_;
        std::atomic<LoggerMode> logger_mode_{LoggerMode::DEFAULT};
        std::atomic<int32_t> log_call_count_{0};    // 记录当前有多少并发的对底层logger的调用
        log::Level min_level_{log::Level::INFO};
        std::unique_ptr<SyncLoggerDetail> sync_logger_;
        std::unique_ptr<AsyncLoggerDetail> async_logger_;
    };

    inline BaseLogger* getBaseLogger()
    {
        static BaseLogger logger;
        return &logger;
    }

    _MEIDO_NOREMOVE BaseLogger* g_keep_base_logger = getBaseLogger();
    static BaseLogger* g_base_logger = getBaseLogger();


    inline const std::string& getDeprecatedWarningStr()
    {
        static std::string str = "[WARNING][%s: line %d] ";
        return str;
    }

    _MEIDO_NOINLINE inline const char* immutableGetDynamicVersion()
    {
        return BOKUMEIDOCPP_VERSION;
    }

    // 如果lib库将bokumeidocpp内接口设置为可见的，那么就需要检查库和可执行文件内的版本是否一致，因为内存布局可能变化
    _MEIDO_HIDDEN inline const char* checkLibExeVersion()
    {
        if (strcmp(BOKUMEIDOCPP_VERSION, immutableGetDynamicVersion()) != 0)
        {
            constexpr const char content_fmt[] = "\n\n"
                                                 "=====================================================\n"
                                                 "  Warning: Multiple versions of bokumeidocpp detected!       \n"
                                                 "-----------------------------------------------------\n"
                                                 "  version a: %-40s|\n"
                                                 "  version b: %-40s|\n"
                                                 "-----------------------------------------------------\n"
                                                 "  This violates ODR and causes undefined behavior!   \n"
                                                 "  Please ensure only one version of bokumeidocpp is linked.  \n"
                                                 "=====================================================\n\n";
            char buf[512];
            snprintf(buf, sizeof(buf) - 1, content_fmt, BOKUMEIDOCPP_VERSION, immutableGetDynamicVersion());
            _MEIDO_WARN_RAW(buf);
        }
        return "";
    }
    _MEIDO_NOREMOVE const char* g_check_libexever = _priv::checkLibExeVersion();

    inline volatile const char* keepVersionString()
    {
        static volatile const char version_str[64] = "using bokumeidocpp version: " BOKUMEIDOCPP_VERSION;
        return version_str;
    }
    static volatile const char* g_keep_verstr = _priv::keepVersionString();


    inline const char* splitFuncName(const char* func_sig, const char* func_name)
    {
        thread_local std::unordered_map<const char*, std::string> func_name_map;
        auto it = func_name_map.find(func_sig);
        if (it != func_name_map.end())
            return it->second.c_str();
        if (strcmp(func_sig, func_name) == 0)
            return func_name;

        std::string s_func_sig = func_sig;
        size_t name_pos = s_func_sig.find(func_name + std::string("("));
        if (name_pos == std::string::npos)
        {
            name_pos = s_func_sig.find(func_name + std::string("<"));
            // 处理gcc上lambda函数
            if (name_pos == std::string::npos)
            {
                s_func_sig.append("::").append(func_name);
                func_name_map[func_sig] = std::move(s_func_sig);
                return func_name_map[func_sig].c_str();
            }
        }
        s_func_sig.erase(name_pos + strlen(func_name));
        s_func_sig.erase(0, s_func_sig.rfind(' ', name_pos - 1) + 1);
        func_name_map[func_sig] = std::move(s_func_sig);
        return func_name_map[func_sig].c_str();
    }



}    // namespace _priv



namespace base
{
    // ==================== ScopeGuard 成员函数实现 ====================
    template <class Callable>
    template <class Cb, decltype(std::declval<Cb&>()(), 0),
              typename std::enable_if<std::is_move_constructible<Cb>::value && !std::is_reference<Cb>::value, int>::type>
    inline ScopeGuard<Callable>::ScopeGuard(Cb callable) :
        valid_(true),
        callable_(std::move(callable))
    {}

    template <class Callable>
    inline void ScopeGuard<Callable>::dismiss() noexcept
    {
        valid_ = false;
    }

    template <class Callable>
    inline void ScopeGuard<Callable>::reset()
    {
        if (valid_)
        {
            callable_();
            valid_ = false;
        }
    }

    template <class Callable>
    inline ScopeGuard<Callable>::ScopeGuard(ScopeGuard&& other) noexcept
        : valid_(other.valid_),
          callable_(std::move(other.callable_))
    {
        other.valid_ = false;
    }

    template <class Callable>
    inline ScopeGuard<Callable>::~ScopeGuard()
    {
        this->reset();
    }

    template <class Callable, decltype(std::declval<Callable&>()(), 0),
              typename std::enable_if<std::is_move_constructible<Callable>::value && !std::is_reference<Callable>::value, int>::type>
    inline ScopeGuard<Callable> makeScopeGuard(Callable callable)
    {
        return ScopeGuard<Callable>(std::move(callable));
    }

    // ==================== getVersion / logVersion ====================

    inline constexpr const char* getVersion()
    {
        return "bokumeidocpp-" BOKUMEIDOCPP_VERSION;
    }

    inline void logVersion(const char* project_name)
    {
        constexpr const char ver[] = "bokumeidocpp-" BOKUMEIDOCPP_VERSION;
        std::string sep_line;
        char buf[256];
        if (project_name)
        {
            sep_line.resize(sizeof(ver) + strlen(project_name) + 10, '=');
            snprintf(buf, sizeof(buf) - 1, "%s\n  %s using %s  \n%s\n", sep_line.c_str(), project_name, ver, sep_line.c_str());
            _MEIDO_INFO_RAW(buf);
        }
        else
        {
            sep_line.resize(sizeof(ver) + 5, '=');
            snprintf(buf, sizeof(buf) - 1, "%s\n  %s  \n%s\n", sep_line.c_str(), ver, sep_line.c_str());
            _MEIDO_INFO_RAW(buf);
        }
    }

    inline constexpr bool isBigEndian()
    {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
        // 编译器内建（GCC / Clang / ICC / TI …），最优先
        return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
#elif defined(_MSC_VER)
        // MSVC: Windows 桌面/x64/ARM/A0RM64 全小端 ABI
        // _M_PPC 等历史大端目标已不在支持的 Windows 行列
        return false;
#elif defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
        return true;
#elif defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
        return false;
#else
        // 新平台/新编译器没覆盖到，直接编译失败，强迫人来检查
        static_assert(false,
                      "Unknown endianness: please update isBigEndian() for this target");
        return false;    // unreachable, keeps compiler happy
#endif
    }

}    // namespace base
}    // namespace meido

#endif    // !BASE_HPP_BOKUMEIDOCPP
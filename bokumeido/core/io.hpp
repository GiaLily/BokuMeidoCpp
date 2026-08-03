/*  bokumeidocpp库的终端输入输出相关 */
#pragma once
#ifndef IO_HPP_BOKUMEIDOCPP
#define IO_HPP_BOKUMEIDOCPP

#include <array>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <set>
#include <vector>

#include <ctype.h>
#include <stdio.h>

#include "base.hpp"
#include "str.hpp"
#include "type.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace io
{
    /*  实现类似Python的print打印功能，基于std::cout
        - 基于std::ostringstream实现，可以接收任意数量和类型的参数
        - 支持重载了std::ostream& operator<<(std::ostream&, const T&)的T对象，operator<<中不可再调用此print
        - 扩展了对非volatile限定的STL可遍历容器对象的支持
        - 有无符号的char都会被当作字符处理
        - 宽字符会被当作数字处理，wstring会被当作装有宽字符的容器（{65, 66, ...}）
        - 未支持的类型会被转换为<ClassName: Address>形式的字符串
        - 在不混用print函数和std::cout时，线程安全  */
    template <class T, class... Args>
    void print(const T& arg, const Args&... args);

    struct BooleanOption
    {
        std::string sflag;          // 短标签，如 -h
        std::string lflag;          // 长标签，如 --help
        std::string description;    // 描述，如 "Show help"
    };

    struct ValueOption
    {
        std::string sflag;            // 短标签，如 -w
        std::string lflag;            // 长标签，如 --width
        std::string description;      // 描述，如 "Set the width"
        std::string default_value;    // 默认值，如 "1280"
    };


    /*  main函数的参数解析工具，只支持解析ASCII字符
        - 先调用parse，再调用其他方法
        - 短标志以单横线 '-'起始，接ASCII单字母，如 -s
        - 长标志以双横线 '--'起始，后续至少两位，首位接ASCII字母，后续位支持ASCII字母数字下划线及分隔符'-'，如 --flag, --long-flag
        - 支持短标志合并，但值选项只能作为合并的最后一位，如 -v value -b -B 合并为 -bBv value  
        - main函数的参数中，未在预设中的标志会被忽略，值选项的值禁止以'-'开头 */
    class ArgumentParser final
    {
    public:
        ArgumentParser();

        /*  解析main函数接收的参数，不在preset中的参数无法被解析
            - 示例：
                int ret = parser.parse(argc, argv, {
                    {"-b", "--bool_key", "bool key"}, 
                    {"", "--bool_key2", "bool key2"}}, {
                    {"-w", "--width", "width of the output", "1280"},
                    {"-h", "--height", "height of the output", "720"}});

            @param boolopts_preset: 预设的布尔选项参数，格式为{{"-s", "--longflag", "description"}, ...}，短标志和长标志至少需要给出一个
            @param valueopts_preset: 预设的值选项参数，格式为{{"-s", "--longflag", "description", "default value"}, ...}，短标志和长标志至少需要给出一个，默认值为空代表必需由用户提供的参数
            @return 0代表成功，负数代表预设或命令行参数格式有误，正数代表正常解析但未解析到必需提供的值参数  */
        int parse(int argc, char* argv[], std::vector<BooleanOption> boolopts_preset, std::vector<ValueOption> valueopts_preset);

        // 获取布尔选项flag的解析结果，注意flag必须为"-b"或"--bool"形式
        bool getBoolOpt(const std::string& flag) const;

        // 获取值选项flag的解析结果，生命周期为ArgumentParser对象销毁或重新parse之前。注意flag必须为"-a"或"--arg"形式
        const std::string& getValueOpt(const std::string& flag) const;

        /*  按一定格式打印预设的选项与描述
            Preset Boolean Options:
                -shortflag  --longflag    Description: description
                ...
            Preset Value Options:
                -shortflag  --longflag    Description: description    Default VALUE: default value
                -shortflag  --longflag    Description: description    Required
                ...                                                             */
        void logPreset() const;

        /*  按一定格式打印解析后的选项值
            Parsed Boolean Options:
                -shortflag  --longflag    Parsed boolean value: true
                -shortflag  --longflag    Parsed boolean value: false
                ...
            Parsed Value Options:
                -shortflag  --longflag    Parsed value: parsed value
                -shortflag  --longflag    No value parsed!
                ...                                                             */
        void logParsed() const;

        // 禁止拷贝和移动
        ArgumentParser(const ArgumentParser& tmp) = delete;
        ArgumentParser& operator=(const ArgumentParser& tmp) = delete;

    private:
        bool checkPresetsAreValid(const std::vector<BooleanOption>& boolopts_preset, const std::vector<ValueOption>& valueopts_preset) const;

        std::vector<BooleanOption> boolopts_preset_;    // 保留预设参数的顺序
        std::vector<ValueOption> valueopts_preset_;
        std::set<std::string> boolopts_parsed_;
        std::map<std::string, std::string> valueopts_parsed_;
        size_t max_flag_size_;
        const std::string empty_str_;
    };


    /*  读写ini文件
        - 文件以UTF-8编码和UTF-8 BOM格式读取
        - 支持带section和不带section的ini文件
        - section和key必须以字母为起始，其他字符可为字母、数字、下划线和短横线'-'，否则该行会被忽略
        - section和key前后可以有空白字符，它们会被自动忽略
        - section由中括号[]包裹，如果有多个同名section，会被当成一个section处理
        - 如果同一个section中有多个同名key，最后一个key会覆盖前面的key
        - key-value行的第一个分隔符到注释标记之间的内容，除去首尾的空白字符都属于value
        - 空白字符指" \t\n\r\f\v"
        - 参考格式如下，不包含section和key-value的行会被忽略：
            [section1]
            key1 = value1
            key2 = value2
            [section2]
            key3 = value3
            key4 = value4
            ...                     */
    class IniFile final
    {
    public:
        IniFile() = default;

        /*  打开ini文件，不存在的文件无法打开
            @param path: 文件路径
            @param key_value_sep: 分割key和value的字符，默认为'='
            @param note_sign: 注释标记符，默认为'#'
            @return 0代表正常，其他代表失败   */
        int open(std::string path, char key_value_sep = '=', char note_sign = '#');

        // 关闭并保存文件，文件已关闭也能close；返回0代表正常，其他代表失败
        int close();

        // 通过section和key获取value的值，section为空代表无section
        std::string getValue(const std::string& section, const std::string& key);

        // 通过key获取value的值，只能获取无section的key-value条目
        std::string getValue(const std::string& key);

        // 设置和添加key-value条目，，section为空代表无section，value可以是任意正确支持std::cout<<的类型
        template <class T, typename std::enable_if<type::StdCoutEachChecker<T>::value, int>::type = 0>
        void setValue(const std::string& section, const std::string& key, const T& value);

        // 设置和添加无section的key-value条目，value可以是任意正确支持std::cout<<的类型
        template <class T, typename std::enable_if<type::StdCoutEachChecker<T>::value, int>::type = 0>
        void setValue(const std::string& key, const T& value);

        // 打印读取的文件内容至日志输出
        void logContents() const;

        // 禁止拷贝和移动
        IniFile(const IniFile& file) = delete;
        IniFile& operator=(const IniFile& file) = delete;
        ~IniFile();

    private:
        struct SectionInfo;
        struct KeyInfo;

        bool searchSection(const std::string& line, SectionInfo& section_info, size_t offset) const;
        bool searchKey(const std::string& line, KeyInfo& key_info, size_t offset) const;
        static bool checkSectionKeyFmt(const std::string& section_or_key);
        int saveContents();

        std::string file_path_;
        std::fstream file_;
        char rwstatus_ = 'r';
        char sep_ = '=';
        char note_sign_ = '#';
        std::list<std::string> content_list_;
        std::map<std::string, SectionInfo> section_map_;
        std::map<std::string, std::map<std::string, KeyInfo>> key_map_;
    };
}    // namespace io










/*--------------------------------------------内部实现--------------------------------------------*/

namespace _priv
{
    _MEIDO_EXPORT inline std::mutex& immutableGetPrintlock()
    {
        static std::mutex lk;
        return lk;
    }
}    // namespace _priv



namespace io
{

    template <class T, class... Args>
    inline void print(const T& arg, const Args&... args)
    {
        _priv::osInputFloatPrecision() = -1;
        _priv::AutoOStream aos(&std::cout);
        {
            std::lock_guard<std::mutex> lk(_priv::immutableGetPrintlock());
            _priv::osInput(aos, arg);
            int tmp[] = {0, (aos.write(" ", 1), _priv::osInput(aos, args), 0)...};
            (void)tmp;
        }
        aos.flush();
        std::cout << std::endl;
    }


    inline ArgumentParser::ArgumentParser()
    {
        max_flag_size_ = 0;
    }

    inline int ArgumentParser::parse(int argc, char* argv[], std::vector<BooleanOption> boolopts_preset, std::vector<ValueOption> valueopts_preset)
    {
        if (!this->checkPresetsAreValid(boolopts_preset, valueopts_preset))
            return -1;
        std::string cmd_str;
        for (int i = 1; i < argc; i++)
        {
            cmd_str.append(argv[i]).append(" ");
        }
        MEIDO_INFO("Parsing command line: {} {}", argv[0], cmd_str);
        max_flag_size_ = 0;
        boolopts_preset_ = std::move(boolopts_preset);
        valueopts_preset_ = std::move(valueopts_preset);

        std::vector<int> boolop_values;    // 存放布尔开关的值
        boolop_values.reserve(boolopts_preset_.size());
        std::map<std::string, int*> boolop_keys;    // 存放布尔开关的键

        size_t now_vsize;
        for (auto& bop : boolopts_preset_)
        {
            boolop_values.push_back(0);
            boolop_keys[bop.sflag] = &boolop_values.back();
            boolop_keys[bop.lflag] = &boolop_values.back();

            if (bop.sflag.empty() || bop.lflag.empty())
                now_vsize = bop.sflag.size() + bop.lflag.size();
            else
                now_vsize = bop.sflag.size() + bop.lflag.size() + 2;
            max_flag_size_ = now_vsize > max_flag_size_ ? now_vsize : max_flag_size_;
        }

        std::vector<std::string> valueop_values;
        valueop_values.reserve(valueopts_preset_.size());
        std::map<std::string, std::string*> valueop_keys;

        for (auto& vop : valueopts_preset_)
        {
            valueop_values.push_back(vop.default_value);
            valueop_keys[vop.sflag] = &valueop_values.back();
            valueop_keys[vop.lflag] = &valueop_values.back();

            if (vop.sflag.empty() || vop.lflag.empty())
                now_vsize = vop.sflag.size() + vop.lflag.size();
            else
                now_vsize = vop.sflag.size() + vop.lflag.size() + 2;

            max_flag_size_ = now_vsize > max_flag_size_ ? now_vsize : max_flag_size_;
        }

        if (argc < 1)
        {
            MEIDO_ERROR("Wrong value of argc:{}", argc);
            return -1;
        }
        for (int i = 1; i < argc; i++)
        {
            std::string flag_maybe = argv[i];
            if (flag_maybe.size() < 2)
                continue;
            if (flag_maybe.data()[0] == '-' && flag_maybe.data()[1] != '-' && flag_maybe.size() > 2)
            {
                for (size_t j = 1; j < flag_maybe.size() - 1; j++)
                {
                    std::string sflag = std::string("-").append(flag_maybe.substr(j, 1));
                    if (boolop_keys.find(sflag) != boolop_keys.end())
                        *boolop_keys[sflag] = 1;
                }
                flag_maybe = std::string("-").append(flag_maybe.substr(flag_maybe.size() - 1, 1));
            }
            if (boolop_keys.find(flag_maybe) != boolop_keys.end())
            {
                *boolop_keys[flag_maybe] = 1;
            }
            else if (valueop_keys.find(flag_maybe) != valueop_keys.end())
            {
                // 如果value option是最后一个argv，或value option的下一个argv是另一个opthion，init失败
                if (i < argc - 1 && argv[i + 1][0] != '-')
                {
                    *valueop_keys[flag_maybe] = argv[i + 1];
                    i++;
                }
                else
                {
                    MEIDO_ERROR("Invalid value of option {}! Please check command line arguments", flag_maybe);
                    return -1;
                }
            }
        }
        boolopts_parsed_.clear();
        valueopts_parsed_.clear();
        for (auto& bkey : boolop_keys)
        {
            if (*bkey.second)
                boolopts_parsed_.emplace(bkey.first);
        }
        for (auto& vkey : valueop_keys)
        {
            valueopts_parsed_[vkey.first] = *vkey.second;
        }
        for (const auto& vop : valueopts_parsed_)
        {
            if (vop.second.empty())
            {
                MEIDO_WARN("Required value option {} is missing", vop.first);
                return 1;
            }
        }

        return 0;
    }

    inline bool ArgumentParser::getBoolOpt(const std::string& flag) const
    {
        if (flag.empty())
        {
            MEIDO_WARN("Got an empty flag");
            return false;
        }
        return boolopts_parsed_.find(flag) != boolopts_parsed_.end();
    }

    inline const std::string& ArgumentParser::getValueOpt(const std::string& flag) const
    {
        if (flag.empty())
        {
            MEIDO_WARN("Got an empty flag");
            return empty_str_;
        }
        if (valueopts_parsed_.find(flag) == valueopts_parsed_.end())
            return empty_str_;
        return valueopts_parsed_.at(flag);
    }

    inline void ArgumentParser::logPreset() const
    {
        if (!boolopts_preset_.empty())
            MEIDO_INFO_RAW("Preset Boolean Options:");
        for (auto& bop : boolopts_preset_)
        {
            std::string flag_part;
            if (bop.sflag.empty() || bop.lflag.empty())
                flag_part = bop.sflag + bop.lflag;
            else
                flag_part = bop.sflag + ", " + bop.lflag;
            flag_part.resize(max_flag_size_, ' ');
            MEIDO_INFO_RAW("    {}    {}", flag_part, bop.description.empty() ? "" : ("[Description] " + bop.description));
        }

        if (!valueopts_preset_.empty())
            MEIDO_INFO_RAW("Preset Value Options:");
        for (auto& vop : valueopts_preset_)
        {
            std::string flag_part;
            if (vop.sflag.empty() || vop.lflag.empty())
                flag_part = vop.sflag + vop.lflag;
            else
                flag_part = vop.sflag + ", " + vop.lflag;
            flag_part.resize(max_flag_size_, ' ');
            MEIDO_INFO_RAW("    {}    {} {}", flag_part, vop.description.empty() ? "" : ("[Description] " + vop.description), vop.default_value.empty() ? "(Required)" : ("(Default: " + vop.default_value + ")"));
        }
        MEIDO_INFO_RAW("");
    }

    inline void ArgumentParser::logParsed() const
    {
        if (!boolopts_preset_.empty())
            MEIDO_INFO_RAW("Parsed Boolean Options:");
        for (auto& bop : boolopts_preset_)
        {
            std::string flag_part;
            if (bop.sflag.empty() || bop.lflag.empty())
                flag_part = bop.sflag + bop.lflag;
            else
                flag_part = bop.sflag + ", " + bop.lflag;
            flag_part.resize(max_flag_size_, ' ');
            std::string flag = bop.sflag.empty() ? bop.lflag : bop.sflag;
            MEIDO_INFO_RAW("    {}    Value: {}", flag_part, boolopts_parsed_.find(flag) != boolopts_parsed_.end() ? "true" : "false");
        }

        if (!valueopts_preset_.empty())
            MEIDO_INFO_RAW("Parsed Value Options:");
        for (auto& vop : valueopts_preset_)
        {
            std::string flag_part;
            if (vop.sflag.empty() || vop.lflag.empty())
                flag_part = vop.sflag + vop.lflag;
            else
                flag_part = vop.sflag + ", " + vop.lflag;
            flag_part.resize(max_flag_size_, ' ');
            std::string flag = vop.sflag.empty() ? vop.lflag : vop.sflag;
            MEIDO_INFO_RAW("    {}    Value: {}", flag_part, valueopts_parsed_.at(flag));
        }
        MEIDO_INFO_RAW("");
    }

    inline bool ArgumentParser::checkPresetsAreValid(const std::vector<BooleanOption>& boolopts_preset, const std::vector<ValueOption>& valueopts_preset) const
    {
        auto func_shortflag_check = [](const std::string& flag) {
            if (flag.size() != 2 || flag[0] != '-' || !isalpha(flag[1]))
                return false;
            return true;
        };

        auto func_longflag_check = [](const std::string& flag) {
            if (flag.size() < 4 || flag[0] != '-' || flag[1] != '-' || !isalpha(flag[2]))
                return false;
            for (size_t i = 3; i < flag.size(); ++i)
            {
                char c = flag[i];
                if (!isalnum(c) && c != '-' && c != '_')
                    return false;
            }
            return true;
        };
        std::set<std::string> bool_flags;
        for (auto& bop : boolopts_preset)
        {
            if (bop.sflag.empty() && bop.lflag.empty())
            {
                MEIDO_ERROR("Both short flag and long flag are empty");
                return false;
            }

            if (!bop.sflag.empty())
            {
                if (!func_shortflag_check(bop.sflag))
                {
                    MEIDO_ERROR("Invalid short flag:{}", bop.sflag);
                    return false;
                }
                if (bool_flags.find(bop.sflag) != bool_flags.end())
                {
                    MEIDO_ERROR("Found duplicate boolean option flag:{}", bop.sflag);
                    return false;
                }
                bool_flags.emplace(bop.sflag);
            }

            if (!bop.lflag.empty())
            {
                if (!func_longflag_check(bop.lflag))
                {
                    MEIDO_ERROR("Invalid long flag:{}", bop.lflag);
                    return false;
                }
                if (bool_flags.find(bop.lflag) != bool_flags.end())
                {
                    MEIDO_ERROR("Found duplicate boolean option flag:{}", bop.lflag);
                    return false;
                }
                bool_flags.emplace(bop.lflag);
            }
        }

        std::set<std::string> value_flags;
        for (auto& vop : valueopts_preset)
        {
            if (vop.sflag.empty() && vop.lflag.empty())
            {
                MEIDO_ERROR("Both short flag and long flag are empty");
                return false;
            }

            if (!vop.sflag.empty())
            {
                if (!func_shortflag_check(vop.sflag))
                {
                    MEIDO_ERROR("Invalid short flag:{}", vop.sflag);
                    return false;
                }
                if (bool_flags.find(vop.sflag) != bool_flags.end())
                {
                    MEIDO_ERROR("The same operation:{} is not allowed in both boolopts_preset and valueopts_preset", vop.sflag);
                    return false;
                }
                if (value_flags.find(vop.sflag) != value_flags.end())
                {
                    MEIDO_ERROR("Found duplicate value option flag:{}", vop.sflag);
                    return false;
                }
                value_flags.emplace(vop.sflag);
            }

            if (!vop.lflag.empty())
            {
                if (!func_longflag_check(vop.lflag))
                {
                    MEIDO_ERROR("Invalid long flag:{}", vop.lflag);
                    return false;
                }
                if (bool_flags.find(vop.lflag) != bool_flags.end())
                {
                    MEIDO_ERROR("The same operation:{} is not allowed in both boolopts_preset and valueopts_preset", vop.lflag);
                    return false;
                }
                if (value_flags.find(vop.lflag) != value_flags.end())
                {
                    MEIDO_ERROR("Found duplicate value option flag:{}", vop.lflag);
                    return false;
                }
                value_flags.emplace(vop.lflag);
            }
        }
        return true;
    }


    struct IniFile::SectionInfo
    {
        std::list<std::string>::iterator line;
        std::list<std::string>::iterator last;
        size_t pos = std::string::npos;
        size_t len = 0;
    };

    struct IniFile::KeyInfo
    {
        std::list<std::string>::iterator line;
        size_t key_pos = std::string::npos;
        size_t key_len = 0;
        size_t value_pos = std::string::npos;
        size_t value_len = 0;
    };

    // 打开ini文件
    inline int IniFile::open(std::string path, char key_value_sep, char note_sign)
    {
        /*  ios::app：　　　 //以追加的方式打开文件
            ios::ate：　　　 //文件打开后定位到文件尾，ios:app就包含有此属性
            ios::binary：　 //以二进制方式打开文件，缺省的方式是文本方式。两种方式的区别见前文
            ios::in：　　　  //文件以输入方式打开（文件数据输入到内存）
            ios::out：　　　 //文件以输出方式打开（内存数据输出到文件）
            ios::nocreate： //不建立文件，所以文件不存在时打开失败
            ios::noreplace：//不覆盖文件，所以打开文件时如果文件存在失败
            ios::trunc：　  //如果文件存在，把文件长度设为0   */

        if (file_.is_open())
        {
            MEIDO_WARN("Duplicated open! Ignore the second open request");
            return 1;
        }

        if (key_value_sep == '[' || key_value_sep == ']')
        {
            MEIDO_ERROR("Invalid key-value separator: {}", key_value_sep);
            return -1;
        }
        if (note_sign == '[' || note_sign == ']')
        {
            MEIDO_ERROR("Invalid note sign: {}", note_sign);
            return -1;
        }
        if (key_value_sep == note_sign)
        {
            MEIDO_ERROR("Key-value separator and note sign cannot be the same character: {}", key_value_sep);
            return -1;
        }

        file_.open(path, std::ios::binary | std::ios::in);
        if (!file_.is_open())
        {
            MEIDO_ERROR("Failed to open {}! Please check if the file exists", path);
            return -1;
        }
        else
        {
            file_path_ = std::move(path);
            sep_ = key_value_sep;
            note_sign_ = note_sign;

            std::string line;
            std::vector<std::string> line_split;
            // content_list_.clear();
            int line_id = -1;
            std::string now_section;
            while (file_.good())    // 按行读取内容，并去掉\r和\n符号
            {
                size_t offset = 0;
                line_id++;
                line.clear();
                std::getline(file_, line);
                if (line_id == 0)
                {
                    if (line.size() > 3 && line.substr(0, 3) == "\xEF\xBB\xBF")
                    {
                        MEIDO_INFO("Process the file:{} with UTF-8 BOM encoding", file_path_);
                        offset = 3;
                    }
                    else
                        MEIDO_INFO("Process the file:{} with UTF-8 encoding", file_path_);
                }
                if (!line.empty())
                {
                    line = str::rtrim(std::move(line));
                }
                content_list_.emplace_back(line);
                if (line.empty())
                    continue;
                SectionInfo section_info;
                if (this->searchSection(line, section_info, offset))
                {
                    now_section = line.substr(section_info.pos, section_info.len);
                    if (section_map_.find(now_section) != section_map_.end())
                    {
                        MEIDO_WARN("Duplicate section:{} at line:{}", now_section, line_id + 1);
                    }
                    section_info.line = --content_list_.end();
                    section_info.last = --content_list_.end();

                    section_map_[now_section] = section_info;
                    key_map_[now_section] = {};
                    continue;
                }

                KeyInfo key_info;
                if (this->searchKey(line, key_info, offset))
                {
                    section_map_[now_section].last = --content_list_.end();

                    std::string key = line.substr(key_info.key_pos, key_info.key_len);

                    if (key_map_.find(now_section) != key_map_.end() && key_map_[now_section].find(key) != key_map_[now_section].end())
                    {
                        MEIDO_WARN("Duplicate key:{} in section:{} at line:{}", key, now_section, line_id + 1);
                    }
                    key_info.line = --content_list_.end();
                    key_map_[now_section][key] = key_info;
                    continue;
                }
            }
            while (true)
            {
                if (!content_list_.empty() && content_list_.back().empty())
                    content_list_.pop_back();
                else
                    break;
            }
            return 0;
        }
    }

    // 关闭并保存
    inline int IniFile::close()
    {
        if (rwstatus_ == 'w' && file_.is_open())
        {
            if (this->saveContents() != 0)
                return -1;
        }
        content_list_.clear();
        section_map_.clear();
        key_map_.clear();
        file_.close();
        _priv::fsyncByPath(file_path_.c_str());
        rwstatus_ = 'r';
        file_.clear();
        file_path_.clear();
        return 0;
    }

    inline std::string IniFile::getValue(const std::string& section, const std::string& key)
    {
        if (!file_.is_open())
        {
            MEIDO_ERROR("File not opened");
            return std::string();
        }
        if (key_map_.find(section) == key_map_.end())
        {
            MEIDO_WARN("The section:{} is not exist! Please check it", section);
            return std::string();
        }
        if (key_map_[section].find(key) == key_map_[section].end())
        {
            MEIDO_WARN("The key:{} is not exist! Please check it", key);
            return std::string();
        }
        KeyInfo& key_info = key_map_[section][key];
        // std::string& line = *key_info.line;
        return (*key_info.line).substr(key_info.value_pos, key_info.value_len);
    }

    inline std::string IniFile::getValue(const std::string& key)
    {
        return this->getValue(std::string(), key);
    }

    template <class T, typename std::enable_if<type::StdCoutEachChecker<T>::value, int>::type>
    inline void IniFile::setValue(const std::string& section, const std::string& key, const T& value)
    {
        if (!file_.is_open())
        {
            MEIDO_ERROR("File not opened");
            return;
        }
        rwstatus_ = 'w';
        if (key_map_.find(section) == key_map_.end())
        {
            if (section.empty())
            {
                std::string value_str = str::toStr(value);
                content_list_.emplace_back(key + sep_ + value_str);
                section_map_[section].last = --content_list_.end();

                key_map_[section][key].line = --content_list_.end();
                key_map_[section][key].key_pos = 0;
                key_map_[section][key].key_len = key.size();
                key_map_[section][key].value_pos = key.size() + 1;
                key_map_[section][key].value_len = value_str.size();
            }
            else
            {
                content_list_.emplace_back("[" + section + "]");
                section_map_[section].line = --content_list_.end();
                section_map_[section].last = --content_list_.end();
                section_map_[section].pos = 1;
                section_map_[section].len = section.size();
                key_map_[section] = {};
                this->setValue(section, key, value);
            }
        }
        else
        {
            if (key_map_[section].find(key) == key_map_[section].end())
            {
                std::string value_str = str::toStr(value);
                auto bak = section_map_[section].last;
                key_map_[section][key].line = content_list_.emplace(++section_map_[section].last, key + sep_ + value_str);
                section_map_[section].last = ++bak;
                key_map_[section][key].key_pos = 0;
                key_map_[section][key].key_len = key.size();
                key_map_[section][key].value_pos = key.size() + 1;
                key_map_[section][key].value_len = value_str.size();
            }
            else
            {
                std::string value_str = str::toStr(value);
                *key_map_[section][key].line = key + sep_ + value_str;
                key_map_[section][key].value_len = value_str.size();
            }
        }
    }

    inline void IniFile::logContents() const
    {
        MEIDO_INFO_RAW("IniFile {}", file_path_);
        for (const auto& line : content_list_)
        {
            MEIDO_INFO_RAW("    {}", line);
        }
        MEIDO_INFO_RAW("");
    }

    template <class T, typename std::enable_if<type::StdCoutEachChecker<T>::value, int>::type>
    inline void IniFile::setValue(const std::string& key, const T& value)
    {
        this->setValue(std::string(), key, value);
    }

    inline IniFile::~IniFile()
    {
        this->close();
    }

    inline bool IniFile::searchSection(const std::string& line, SectionInfo& section_info, size_t offset) const
    {
        // 找到注释的位置
        size_t note_pos = line.size();
        size_t tmp_pos = line.find(note_sign_);
        if (tmp_pos < note_pos)
            note_pos = tmp_pos;

        size_t pos0 = line.find_first_not_of(" \t\n\r\f\v", offset);
        if (pos0 >= note_pos || line[pos0] != '[')
            return false;
        size_t pos1 = line.find_last_not_of(" \t\n\r\f\v", note_pos - 1);
        if (pos1 >= note_pos || line[pos1] != ']')
            return false;
        if (pos1 <= pos0 + 1)
            return false;

        size_t sec_pos0 = line.find_first_not_of(" \t\n\r\f\v", pos0 + 1);
        size_t sec_pos1 = line.find_last_not_of(" \t\n\r\f\v", pos1 - 1);

        if (sec_pos1 >= sec_pos0)
        {
            std::string value = line.substr(sec_pos0, sec_pos1 - sec_pos0 + 1);
            if (!checkSectionKeyFmt(value))
            {
                MEIDO_WARN("Invalid section:{} in line:{}! Ignored", value, line);
                return false;
            }
            section_info.pos = sec_pos0;
            section_info.len = sec_pos1 - sec_pos0 + 1;
            return true;
        }
        else
            return false;
    }

    inline bool IniFile::searchKey(const std::string& line, KeyInfo& key_info, size_t offset) const
    {
        // 找到注释的位置
        size_t note_pos = line.size();
        size_t tmp_pos = line.find(note_sign_);
        if (tmp_pos < note_pos)
            note_pos = tmp_pos;

        size_t sep_pos = line.find(sep_);
        if (sep_pos >= note_pos)
            return false;

        std::string key = str::trim(line.substr(offset, sep_pos - offset), " \t\n\r\f\v");
        if (!checkSectionKeyFmt(key))
        {
            MEIDO_WARN("Invalid key:{} in line:{}! Ignored", key, line);
            return false;
        }

        key_info.key_pos = line.rfind(key, sep_pos);
        key_info.key_len = key.size();

        size_t v_pos0 = line.find_first_not_of(" \t\n\r\f\v", sep_pos + 1);
        size_t v_pos1 = line.find_last_not_of(" \t\n\r\f\v", note_pos - 1);

        if (v_pos1 >= v_pos0)
        {
            std::string value = line.substr(v_pos0, v_pos1 - v_pos0 + 1);
            key_info.value_pos = v_pos0;
            key_info.value_len = v_pos1 - v_pos0 + 1;
            return true;
        }
        else
            return false;
    }

    inline bool IniFile::checkSectionKeyFmt(const std::string& section_or_key)
    {
        if (section_or_key.empty() || !isalnum(static_cast<unsigned char>(section_or_key[0])))
            return false;
        for (size_t i = 1; i < section_or_key.size(); ++i)
        {
            const auto& c = section_or_key[i];
            if (!isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
                return false;
        }
        return true;
    }

    inline int IniFile::saveContents()
    {
        file_.close();
        file_.open(file_path_, std::ios::binary | std::ios::trunc | std::ios::out);
        if (!file_.is_open())
        {
            MEIDO_ERROR("Open {} failed\n", file_path_);
            return -1;
        }
        size_t i = 0;
        for (auto& content : content_list_)
        {
            if (i < content_list_.size() - 1)
                file_ << content << "\n";
            else
                file_ << content;
            i++;
        }
        return 0;
    }

}    // namespace io
}    // namespace meido

#endif    // !IO_HPP_BOKUMEIDOCPP

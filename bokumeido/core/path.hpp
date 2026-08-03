/*  bokumeidocpp库的路径相关功能 */
#pragma once
#ifndef PATH_HPP_BOKUMEIDOCPP
#define PATH_HPP_BOKUMEIDOCPP

#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <stdio.h>
#include <sys/stat.h>

#include "base.hpp"
#include "log.hpp"
#include "type.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

// 输入的路径应为合法格式
namespace path
{
    // 将windows路径中的\\变为标准的/分隔符，去除末尾/分隔符，并将路径标准化，只能正确处理本身合法的路径
    std::string normPath(std::string path);

    // 判断路径是否存在
    bool exists(std::string path);

    // 分割路径字符串的文件或目录名
    std::string splitName(std::string path, bool need_extension = true);

    // 获取路径字符串的后缀名
    std::string splitExt(std::string path);

    // 判断路径是否为绝对路径
    bool isAbs(std::string path);

    // 判断路径是否为存在的目录(将目录的软链接视为目录)
    bool isDir(std::string path);

    // 判断路径是否为存在的普通文件(Windows的快捷方式会被视为文件)
    bool isFile(std::string path);

    // 判断路径是否为存在的文件且拥有给定的后缀名之一
    bool isFileMatchExts(std::string path, const std::set<std::string>& file_exts);

    // 实现类似python的os.path.join功能
    template <class... Strs, typename std::enable_if<type::ConstructibleFromEachChecker<std::string, std::string, Strs...>::value, int>::type = 0>
    std::string join(std::string path1, std::string path2, Strs... paths);

    // 获取目录下的一级文件和目录
    std::vector<std::string> listDir(std::string path, bool return_path = true, const std::set<std::string>& ignore_names = {});

    // 创建目录，操作完成后path目录存在则返回true
    bool requireDirs(std::string path);

    // 创建文件，操作完成后path文件存在则返回true
    bool makeFile(std::string path);

    // 返回路径字符串对应的父目录
    std::string parent(std::string path);

    // 遍历目录下的所有文件，出错时返回空vector
    std::vector<std::string> walk(std::string path, bool return_path = true);

}    // namespace path










/*--------------------------------------------内部实现--------------------------------------------*/

namespace _priv
{
    bool exists(const std::string& path);
    std::string splitName(const std::string& path, bool need_extension);
    std::string splitExt(const std::string& path);
    bool isAbs(const std::string& path);
    bool isFile(const std::string& path);
    bool isFileMatchExts(const std::string& path, const std::set<std::string>& file_exts);
    template <class... Strs>
    inline std::string join(const std::string& path1, const std::string& path2, const Strs&... paths);
    std::vector<std::string> listDir(const std::string& path, bool return_path, const std::set<std::string>& ignore_names);
    bool makeFile(const std::string& path);
    std::string parent(const std::string& path);
    bool remove(const std::string& path);
    std::vector<std::string> walk(const std::string& path, bool return_path);

    inline std::string splitName(const std::string& path, bool need_extension)
    {
        std::string name;
        if (need_extension)
            name = path.substr(path.rfind('/') + 1);
        else
        {
            auto start_pos = path.rfind('/') + 1;
            auto end_pos = path.rfind('.');
            if (end_pos > start_pos)
                name = path.substr(start_pos, end_pos - start_pos);
            else
                name = path.substr(start_pos);
        }
        return name;
    }

    inline std::string splitExt(const std::string& path)
    {
        auto name_pos = path.rfind('/') + 1;
        auto point_pos = path.rfind('.');
        std::string ext;
        if (point_pos == std::string::npos)
            return std::string();
        if (point_pos >= name_pos)
            return path.substr(point_pos + 1);
        return std::string();
    }


    inline bool isFile(const std::string& path)
    {
        struct stat buffer;
#if defined(_MEIDO_WIN32)
        return ::stat(path.c_str(), &buffer) == 0 && ((buffer.st_mode) & S_IFMT) == S_IFREG;
#else
        return ::stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
#endif
    }

    inline bool isFileMatchExts(const std::string& path, const std::set<std::string>& file_exts)
    {
        return _priv::isFile(path) && file_exts.find(_priv::splitExt(path)) != file_exts.end();
    }

    inline std::string joinBranch(const std::string& path)
    {
        return path;
    }
    template <class... Strs>
    inline std::string joinBranch(const std::string& path1, const std::string& path2, const Strs&... paths)
    {
        std::string pth2 = path1 + "/" + path2;
        return _priv::joinBranch(pth2, paths...);
    }

    template <class... Strs>
    inline std::string join(const std::string& path1, const std::string& path2, const Strs&... paths)
    {
        return _priv::joinBranch(path1, path2, paths...);
    }

    inline std::vector<std::string> listDir(const std::string& path, bool return_path, const std::set<std::string>& ignore_names)
    {
        std::vector<std::string> files;
        if (!_priv::isDir(path))
        {
            MEIDO_WARN("The input path:{} is not a folder or does not exist. Please check it", path);
            return {};
        }
#if defined(_MEIDO_WIN32)
        intptr_t hFile = 0;
        struct _finddata_t fileinfo;
        if ((hFile = _findfirst((path + "/*").c_str(), &fileinfo)) != std::string::npos)
        {
            std::string fname;
            do
            {
                fname = fileinfo.name;
                if (fname != "." && fname != ".." && ignore_names.find(fname) == ignore_names.end())
                {
                    // 保存文件的全路径
                    if (return_path)
                        files.push_back(_priv::join(path, fname));
                    else
                        files.push_back(fname);
                }
            } while (_findnext(hFile, &fileinfo) == 0);    // 寻找下一个，成功返回0，否则-1
            ::_findclose(hFile);
        }
#else
        ::DIR* pDir;
        struct ::dirent* ptr;
        pDir = ::opendir(path.c_str());
        std::string fname;
        while ((ptr = ::readdir(pDir)) != nullptr)
        {
            fname = ptr->d_name;
            if (fname != "." && fname != ".." && ignore_names.find(fname) == ignore_names.end())
            {
                if (return_path)
                    files.push_back(_priv::join(path, fname));
                else
                    files.push_back(fname);
            }
        }
        ::closedir(pDir);
#endif
        return files;
    }

    inline bool makeFile(const std::string& path)
    {
        if (_priv::exists(path))
            return true;
        std::string parent_dir;
        if (parent_dir = _priv::parent(path), !parent_dir.empty())
        {
            _priv::requireDirs(parent_dir);
        }
        std::ofstream file(path, std::ios::binary | std::ios::out);
        file.close();
        return _priv::isFile(path);
    }

    inline std::string parent(const std::string& path)
    {
        return path.substr(0, path.find_last_of('/'));
    }

    inline std::vector<std::string> walk(const std::string& path, bool return_path)
    {
        std::vector<std::string> filenames;

        if (!_priv::isDir(path))
        {
            MEIDO_WARN("The input path:{} is not a folder or does not exist. Please check it", path);
            return {};
        }
        std::vector<std::string> listdir_res = _priv::listDir(path, true, {});
        for (std::string& f_d_path : listdir_res)
        {
            if (_priv::isDir(f_d_path))
            {
                std::vector<std::string> sub_filenames = _priv::walk(f_d_path, return_path);
                filenames.insert(filenames.end(), sub_filenames.begin(), sub_filenames.end());
            }
            else
                return_path ? filenames.push_back(f_d_path) : filenames.push_back(_priv::splitName(f_d_path, true));
        }
        return filenames;
    }
}    // namespace _priv



namespace path
{
    inline std::string normPath(std::string path)
    {
        return _priv::normPath(std::move(path));
    }

    inline bool exists(std::string path)
    {
        return _priv::exists(path::normPath(std::move(path)));
    }

    inline std::string splitName(std::string path, bool need_extension)
    {
        return _priv::splitName(path::normPath(std::move(path)), need_extension);
    }


    inline std::string splitExt(std::string path)
    {
        return _priv::splitExt(path::normPath(std::move(path)));
    }

    inline bool isAbs(std::string path)
    {
        return _priv::isAbs(path::normPath(std::move(path)));
    }


    inline bool isDir(std::string path)
    {
        return _priv::isDir(path::normPath(std::move(path)));
    }

    inline bool isFile(std::string path)
    {
        return _priv::isFile(path::normPath(std::move(path)));
    }

    inline bool isFileMatchExts(std::string path, const std::set<std::string>& file_exts)
    {
        return _priv::isFileMatchExts(path::normPath(std::move(path)), file_exts);
    }

    // 实现类似python的os.path.join功能
    template <class... Strs, typename std::enable_if<type::ConstructibleFromEachChecker<std::string, std::string, Strs...>::value, int>::type>
    inline std::string join(std::string path1, std::string path2, Strs... paths)
    {
        return _priv::join(path::normPath(std::move(path1)), path::normPath(std::move(path2)), path::normPath(std::move(paths))...);
    }


    inline std::vector<std::string> listDir(std::string path, bool return_path, const std::set<std::string>& ignore_names)
    {
        return _priv::listDir(path::normPath(std::move(path)), return_path, ignore_names);
    }

    inline bool requireDirs(std::string path)
    {
        return _priv::requireDirs(path::normPath(std::move(path)));
    }

    inline bool makeFile(std::string path)
    {
        return _priv::makeFile(path::normPath(std::move(path)));
    }

    inline std::string parent(std::string path)
    {
        return _priv::parent(path::normPath(std::move(path)));
    }

    inline std::vector<std::string> walk(std::string path, bool return_path)
    {
        return _priv::walk(path::normPath(std::move(path)), return_path);
    }


    //         inline void _removeDir(const std::string& path)
    //         {
    //             std::vector<std::string> listdir_res = _priv::listDir(path, true, {});
    //             for (std::string& sub_path : listdir_res)
    //             {
    //                 if (_priv::isDir(sub_path))
    //                 {
    //                     _priv::removeDir(sub_path);
    //                 }
    //                 else
    //                     remove(sub_path.c_str());
    //             }
    // #if defined(_WIN32)
    //             ::_rmdir(path.c_str());
    // #else
    //             ::rmdir(path.c_str());
    // #endif
    //         }

    // _MEIDO_DEPRECATED("Deprecated! Please use the system api instead.") inline bool remove(std::string path)
    // {
    //     return _privremove(path::normPath(std::move(path)));
    // }
    // inline bool _remove(const std::string& path)
    // {
    //     if (_priv::isDir(path))
    //     {
    //         _priv::removeDir(path);
    //     }
    //     else if (_priv::isFile(path))
    //     {
    //         remove(path.c_str());
    //     }
    //     return !_priv::exists(path);
    // }

}    // namespace path
}    // namespace meido

#endif    // !PATH_HPP_BOKUMEIDOCPP
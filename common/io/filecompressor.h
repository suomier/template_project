#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

/**
 * @brief libarchive 压缩工具类
 *
 * 该类提供将目录压缩为归档文件的功能，支持平台相关的压缩格式：
 * - Windows 平台：ZIP 格式
 * - Unix/Linux/macOS 平台：tar.gz 格式
 *
 * 使用 libarchive 库进行压缩，提供统一的接口，自动处理平台差异。
 */
class FileCompressor
{
public:
    /**
     * @brief 构造函数 - 将文件计数器初始化为零
     */
    FileCompressor();

    /**
     * @brief 将目录压缩为归档文件
     *
     * 根据平台自动选择压缩格式：
     * - Windows: ZIP 格式
     * - Unix/Linux/macOS: tar.gz 格式
     *
     * @param source_dir 要压缩的源目录路径
     * @param output_archive 输出归档文件路径（可选，若为空则自动生成）
     * @return 如果压缩成功返回 true，否则返回 false
     */
    bool CompressDirectory(const std::string &source_dir, const std::string &output_archive = "");

    /**
     * @brief 将多个文件压缩为归档文件
     *
     * 根据平台自动选择压缩格式：
     * - Windows: ZIP 格式
     * - Unix/Linux/macOS: tar.gz 格式
     *
     * @param files 要压缩的文件列表（包含源文件路径和在归档中的相对路径）
     * @param output_archive 输出归档文件路径
     * @return 如果压缩成功返回 true，否则返回 false
     */
    bool CompressFiles(const std::vector<std::pair<std::string, std::string>> &files, const std::string &output_archive);

private:
    /**
     * @brief 根据平台生成默认的输出文件路径
     *
     * @param source_dir 源目录路径
     * @return 默认的输出文件路径
     */
    std::string GetDefaultOutputPath(const std::string &source_dir) const;

    /**
     * @brief 根据平台设置归档格式和压缩方式
     *
     * @param arch libarchive 写入对象
     * @return 如果设置成功返回 true，否则返回 false
     */
    bool SetupArchiveFormat(struct archive *arch) const;

    /**
     * @brief 将目录条目添加到归档
     *
     * @param arch libarchive 写入对象
     * @param relative_path 目录的相对路径
     * @param last_write_time 最后修改时间
     * @return 如果添加成功返回 true，否则返回 false
     */
    bool AddDirectoryToArchive(struct archive *arch, const std::string &relative_path, std::filesystem::file_time_type last_write_time) const;

    /**
     * @brief 将文件添加到归档
     *
     * @param arch libarchive 写入对象
     * @param filepath 源文件的绝对路径
     * @param relative_path 归档中的相对路径
     * @param last_write_time 最后修改时间
     * @return 如果添加成功返回 true，否则返回 false
     */
    bool AddFileToArchive(struct archive *arch, const std::string &filepath, const std::string &relative_path,
                          std::filesystem::file_time_type last_write_time);

    /**
     * @brief 将文件系统时间转换为 time_t
     *
     * @param file_time 文件系统时间
     * @return time_t 时间戳
     */
    time_t FileTimeToTimeT(std::filesystem::file_time_type file_time) const;

private:
    size_t file_count_;
};

#endif // FILECOMPRESSOR_H

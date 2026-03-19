#include "filecompressor.h"

#include <archive.h>
#include <archive_entry.h>

#include <spdlog/spdlog.h>

FileCompressor::FileCompressor()
    : file_count_(0)
{
}

bool FileCompressor::CompressDirectory(const std::string &source_dir, const std::string &output_archive)
{
    spdlog::info("Starting archive compression: {} -> {}", source_dir, output_archive.empty() ? "(auto)" : output_archive);

    if (!std::filesystem::exists(source_dir))
    {
        spdlog::error("Source directory does not exist: {}", source_dir);
        return false;
    }

    // 确定输出文件名
    std::string output_path = output_archive;
    if (output_path.empty())
    {
        output_path = GetDefaultOutputPath(source_dir);
    }

    // 创建归档对象
    struct archive *arch = archive_write_new();
    if (arch == nullptr)
    {
        spdlog::error("Failed to create archive object");
        return false;
    }

    // 根据平台设置压缩格式
    if (!SetupArchiveFormat(arch))
    {
        archive_write_free(arch);
        return false;
    }

    // 打开输出文件
    if (archive_write_open_filename(arch, output_path.c_str()) != ARCHIVE_OK)
    {
        spdlog::error("Failed to open output file: {}", output_path);
        archive_write_free(arch);
        return false;
    }

    // 遍历目录并添加所有文件
    file_count_ = 0;
    try
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(source_dir))
        {
            std::string relative_path = std::filesystem::relative(entry.path(), source_dir).string();
            if (entry.is_directory())
            {
                // 添加目录条目
                AddDirectoryToArchive(arch, relative_path, entry.last_write_time());
            }
            else if (entry.is_regular_file())
            {
                // 添加文件条目
                if (!AddFileToArchive(arch, entry.path().string(), relative_path, entry.last_write_time()))
                {
                    spdlog::error("Failed to add file to archive: {}", entry.path().string());
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error while traversing directory: {}", e.what());
        archive_write_close(arch);
        archive_write_free(arch);
        return false;
    }

    // 关闭归档并释放资源
    archive_write_close(arch);
    archive_write_free(arch);

    spdlog::info("Compression completed! Compressed {} entries to {}", file_count_, output_path);
    return true;
}

std::string FileCompressor::GetDefaultOutputPath(const std::string &source_dir) const
{
    std::string dir_name = std::filesystem::path(source_dir).filename().string();
#ifdef _WIN32
    return dir_name + ".zip";
#else
    return dir_name + ".tar.gz";
#endif
}

bool FileCompressor::SetupArchiveFormat(struct archive *arch) const
{
#ifdef _WIN32
    // Windows 平台使用 ZIP 格式
    spdlog::info("Using ZIP format (Windows platform)");
    archive_write_set_format_zip(arch);
#else
    // Unix/Linux/macOS 平台使用 tar.gz 格式
    spdlog::info("Using tar.gz format (Unix platform)");
    archive_write_add_filter_gzip(arch);
    archive_write_set_format_pax_restricted(arch);
#endif

    return true;
}

bool FileCompressor::AddDirectoryToArchive(struct archive *arch, const std::string &relative_path, std::filesystem::file_time_type last_write_time) const
{
    struct archive_entry *entry = archive_entry_new();
    if (entry == nullptr)
    {
        spdlog::error("Failed to create archive entry for directory: {}", relative_path);
        return false;
    }

    archive_entry_set_pathname(entry, relative_path.c_str());
    archive_entry_set_mode(entry, S_IFDIR | 0755);
    archive_entry_set_mtime(entry, FileTimeToTimeT(last_write_time), 0);

    if (archive_write_header(arch, entry) != ARCHIVE_OK)
    {
        spdlog::error("Failed to write directory header: {}", archive_error_string(arch));
        archive_entry_free(entry);
        return false;
    }

    archive_entry_free(entry);
    return true;
}

bool FileCompressor::AddFileToArchive(struct archive *arch, const std::string &filepath, const std::string &relative_path, std::filesystem::file_time_type last_write_time)
{
    spdlog::info("Adding file: {}", relative_path);

    // 读取文件内容
    std::ifstream in_file(filepath, std::ios::binary);
    if (!in_file.is_open())
    {
        spdlog::error("Failed to read file: {}", filepath);
        return false;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
    in_file.close();

    // 创建归档条目
    struct archive_entry *entry = archive_entry_new();
    if (entry == nullptr)
    {
        spdlog::error("Failed to create archive entry for file: {}", relative_path);
        return false;
    }

    archive_entry_set_pathname(entry, relative_path.c_str());
    archive_entry_set_mode(entry, S_IFREG | 0644);
    archive_entry_set_size(entry, buffer.size());
    archive_entry_set_mtime(entry, FileTimeToTimeT(last_write_time), 0);

    // 写入文件头部
    if (archive_write_header(arch, entry) != ARCHIVE_OK)
    {
        spdlog::error("Failed to write file header: {}", archive_error_string(arch));
        archive_entry_free(entry);
        return false;
    }

    // 写入文件内容
    la_ssize_t bytes_written = archive_write_data(arch, buffer.data(), buffer.size());
    if (bytes_written < 0)
    {
        spdlog::error("Failed to write file data: {}", archive_error_string(arch));
        archive_entry_free(entry);
        return false;
    }

    archive_entry_free(entry);
    file_count_++;
    return true;
}

time_t FileCompressor::FileTimeToTimeT(std::filesystem::file_time_type file_time) const
{
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

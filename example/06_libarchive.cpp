#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <archive.h>
#include <archive_entry.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

/**
 * @brief libarchive 压缩工具类
 *
 * 该类提供将目录压缩为归档文件的功能，支持平台相关的压缩格式：
 * - Windows 平台：ZIP 格式
 * - Unix/Linux/macOS 平台：tar.gz 格式
 *
 * 使用 libarchive 库进行压缩，提供统一的接口，自动处理平台差异。
 */
class ArchiveCompressor
{
public:
    /**
     * @brief 构造函数 - 将文件计数器初始化为零
     */
    ArchiveCompressor()
        : file_count_(0)
    {
    }

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
    bool CompressDirectory(const std::string &source_dir, const std::string &output_archive = "")
    {
        spdlog::info("Starting archive compression: {} -> {}", source_dir, output_archive.empty() ? "(auto)" : output_archive);

        if (!fs::exists(source_dir))
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
            for (const auto &entry : fs::recursive_directory_iterator(source_dir))
            {
                std::string relative_path = fs::relative(entry.path(), source_dir).string();
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

private:
    /**
     * @brief 根据平台生成默认的输出文件路径
     *
     * @param source_dir 源目录路径
     * @return 默认的输出文件路径
     */
    std::string GetDefaultOutputPath(const std::string &source_dir) const
    {
        std::string dir_name = fs::path(source_dir).filename().string();
#ifdef _WIN32
        return dir_name + ".zip";
#else
        return dir_name + ".tar.gz";
#endif
    }

    /**
     * @brief 根据平台设置归档格式和压缩方式
     *
     * @param arch libarchive 写入对象
     * @return 如果设置成功返回 true，否则返回 false
     */
    bool SetupArchiveFormat(struct archive *arch) const
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

    /**
     * @brief 将目录条目添加到归档
     *
     * @param arch libarchive 写入对象
     * @param relative_path 目录的相对路径
     * @param last_write_time 最后修改时间
     * @return 如果添加成功返回 true，否则返回 false
     */
    bool AddDirectoryToArchive(struct archive *arch, const std::string &relative_path,
                               fs::file_time_type last_write_time) const
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
                          fs::file_time_type last_write_time)
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

    /**
     * @brief 将文件系统时间转换为 time_t
     *
     * @param file_time 文件系统时间
     * @return time_t 时间戳
     */
    time_t FileTimeToTimeT(fs::file_time_type file_time) const
    {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            file_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return std::chrono::system_clock::to_time_t(sctp);
    }

private:
    size_t file_count_;
};

/**
 * @brief 创建带有示例文件的测试目录，用于压缩测试
 *
 * 该函数创建一个包含多个文本文件的目录结构，
 * 包括嵌套的子目录，以演示递归压缩功能。
 *
 * @param test_dir 要创建的测试目录路径
 * @return 如果目录创建成功返回 true，否则返回 false
 */
bool CreateTestDirectory(const std::string &test_dir)
{
    try
    {
        fs::create_directories(test_dir);

        // 创建一些测试文件
        std::ofstream(test_dir + "/file1.txt") << "This is the content of the first test file\n";
        std::ofstream(test_dir + "/file2.txt") << "This is the content of the second test file\nHello World!\n";
        std::ofstream(test_dir + "/file3.txt") << "This is the third test file\nContains multiple lines\nThird line content\n";

        // 创建子目录
        std::string subdir = test_dir + "/subfolder";
        fs::create_directories(subdir);
        std::ofstream(subdir + "/nested_file.txt") << "This is a file in the subfolder\n";

        // 创建更深层的子目录
        std::string deep_subdir = subdir + "/deep_folder";
        fs::create_directories(deep_subdir);
        std::ofstream(deep_subdir + "/deep_file.txt") << "This is a deeply nested file\n";

        return true;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Failed to create test directory: {}", e.what());
        return false;
    }
}

/**
 * @brief libarchive 压缩示例的主入口点
 *
 * 通过创建测试目录、将其压缩为平台相关的归档文件并清理测试目录，
 * 演示 ArchiveCompressor 类的用法。
 *
 * 在 Windows 平台创建 .zip 文件，在 Unix/Linux/macOS 平台创建 .tar.gz 文件。
 *
 * @return 成功时返回 0，失败时返回 1
 */
int main(int, char *[])
{
    // 将 spdlog 配置为显示信息级别的消息
    spdlog::set_level(spdlog::level::info);

    // 创建测试目录
    std::string test_dir = "test_archive_source";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output.zip";
    spdlog::info("=== libarchive ZIP Compression Example (Windows) ===");
#else
    output_archive = "test_output.tar.gz";
    spdlog::info("=== libarchive tar.gz Compression Example (Unix) ===");
#endif

    // 创建测试目录和文件
    if (!CreateTestDirectory(test_dir))
    {
        spdlog::error("Failed to create test directory");
        return 1;
    }

    spdlog::info("Test directory created successfully: {}", test_dir);

    // 执行压缩
    ArchiveCompressor compressor;
    if (!compressor.CompressDirectory(test_dir, output_archive))
    {
        spdlog::error("Compression failed");
        return 1;
    }

    spdlog::info("Compression completed! You can verify using decompression tools: {}", output_archive);

    // 清理测试目录（可选）
    spdlog::info("=== Cleaning up test files ===");
    try
    {
        fs::remove_all(test_dir);
        spdlog::info("Test directory deleted");
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Failed to delete test directory: {}", e.what());
    }

    return 0;
}
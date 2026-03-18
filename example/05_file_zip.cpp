#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <zlib.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

/**
 * @brief ZIP 本地文件头结构
 *
 * 该结构表示 ZIP 存档中每个文件前的头部信息。
 * 包含文件的元数据，包括文件名、大小、压缩方法和 CRC32 校验和。
 * 签名值为 0x04034b50。
 */
#pragma pack(push, 1)
struct ZipLocalFileHeader
{
    uint32_t signature; // 0x04034b50
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_length;
};

/**
 * @brief ZIP 中央目录结构
 *
 * 该结构包含 ZIP 存档中央目录中文件的元数据。
 * 允许 ZIP 读取器高效地定位和提取文件。签名值为 0x02014b50。
 */
struct ZipCentralDirectory
{
    uint32_t signature; // 0x02014b50
    uint16_t version_made_by;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_length;
    uint16_t file_comment_length;
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t relative_offset_local_header;
};

/**
 * @brief ZIP 中央目录结束结构
 *
 * 该结构标记 ZIP 中央目录的结束，允许 ZIP 读取器定位中央目录本身。
 * 签名值为 0x06054b50。
 */
struct ZipEndOfCentralDirectory
{
    uint32_t signature; // 0x06054b50
    uint16_t disk_number;
    uint16_t central_dir_disk;
    uint16_t entries_on_disk;
    uint16_t total_entries;
    uint32_t central_dir_size;
    uint32_t central_dir_offset;
    uint16_t comment_length;
};
#pragma pack(pop)

/**
 * @brief ZIP 文件压缩工具类
 *
 * 该类提供将整个目录压缩为标准 ZIP 格式文件的功能。
 * 使用 zlib 的 deflate 算法进行压缩，生成有效的 ZIP 文件结构，
 * 包括本地文件头、中央目录和中央目录结束记录。
 */
class FileZipper
{
public:
    /**
     * @brief 构造函数 - 将文件计数器初始化为零
     */
    FileZipper()
        : file_count_(0)
    {
    }

    /**
     * @brief 将目录压缩为 ZIP 文件
     *
     * 该方法递归遍历源目录，使用 deflate 压缩所有常规文件，
     * 并将它们写入输出 ZIP 文件，具有正确的 ZIP 格式结构。
     *
     * @param source_dir 要压缩的源目录路径
     * @param output_zip 要创建的输出 ZIP 文件路径
     * @return 如果压缩成功返回 true，否则返回 false
     */
    bool CompressDirectory(const std::string &source_dir, const std::string &output_zip)
    {
        spdlog::info("Starting directory compression: {} -> {}", source_dir, output_zip);

        if (!fs::exists(source_dir))
        {
            spdlog::error("Source directory does not exist: {}", source_dir);
            return false;
        }

        out_file_.open(output_zip, std::ios::binary);
        if (!out_file_.is_open())
        {
            spdlog::error("Failed to create output file: {}", output_zip);
            return false;
        }

        central_dir_entries_.clear();
        local_headers_.clear();

        // Traverse directory and compress all files recursively
        try
        {
            for (const auto &entry : fs::recursive_directory_iterator(source_dir))
            {
                if (entry.is_regular_file())
                {
                    std::string relative_path = fs::relative(entry.path(), source_dir).string();
                    if (!AddFileToZip(entry.path().string(), relative_path))
                    {
                        spdlog::error("Failed to compress file: {}", entry.path().string());
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error while traversing directory: {}", e.what());
            out_file_.close();
            return false;
        }

        // Write central directory and end record
        WriteCentralDirectory();

        out_file_.close();
        spdlog::info("Compression completed! Compressed {} files to {}", file_count_, output_zip);
        return true;
    }

private:
    /**
     * @brief 将单个文件添加到 ZIP 存档
     *
     * 读取文件内容，计算 CRC32，使用 deflate 压缩数据，
     * 并将本地文件头和压缩数据写入输出文件。
     * 同时保存中央目录所需的信息。
     *
     * @param filepath 源文件的绝对路径
     * @param relative_path ZIP 存档中文件的相对路径
     * @return 如果文件添加成功返回 true，否则返回 false
     */
    bool AddFileToZip(const std::string &filepath, const std::string &relative_path)
    {
        spdlog::info("Adding file: {}", relative_path);

        // Read file content
        std::ifstream in_file(filepath, std::ios::binary);
        if (!in_file.is_open())
        {
            spdlog::error("Failed to read file: {}", filepath);
            return false;
        }

        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in_file)),
                                    std::istreambuf_iterator<char>());
        in_file.close();

        // Calculate CRC32 checksum
        uint32_t crc = crc32(0, buffer.data(), buffer.size());

        // Compress data using deflate algorithm
        std::vector<uint8_t> compressed_data = CompressData(buffer);

        // Record local file header position
        uint32_t local_header_offset = static_cast<uint32_t>(out_file_.tellp());
        local_headers_.push_back(local_header_offset);

        // Write local file header
        ZipLocalFileHeader header;
        header.signature = 0x04034b50;
        header.version_needed = 20;
        header.flags = 0;
        header.compression = 8; // Deflate
        header.mod_time = 0;
        header.mod_date = 0;
        header.crc32 = crc;
        header.compressed_size = static_cast<uint32_t>(compressed_data.size());
        header.uncompressed_size = static_cast<uint32_t>(buffer.size());
        header.filename_length = static_cast<uint16_t>(relative_path.size());
        header.extra_length = 0;

        out_file_.write(reinterpret_cast<const char *>(&header), sizeof(header));
        out_file_.write(relative_path.c_str(), relative_path.size());
        out_file_.write(reinterpret_cast<const char *>(compressed_data.data()), compressed_data.size());

        // Save central directory information
        CentralDirEntry entry;
        entry.relative_path = relative_path;
        entry.crc32 = crc;
        entry.compressed_size = static_cast<uint32_t>(compressed_data.size());
        entry.uncompressed_size = static_cast<uint32_t>(buffer.size());
        entry.local_header_offset = local_header_offset;
        central_dir_entries_.push_back(entry);

        file_count_++;
        return true;
    }

    /**
     * @brief 使用 zlib 的 deflate 算法压缩数据
     *
     * 使用 deflateInit2 和原始 deflate 模式（-MAX_WBITS）生成适合 ZIP 文件的压缩数据。
     * 分块处理数据以提高内存效率。
     *
     * @param data 要压缩的输入数据
     * @return 压缩后的数据，如果压缩失败则返回原始数据
     */
    std::vector<uint8_t> CompressData(const std::vector<uint8_t> &data)
    {
        z_stream stream;
        memset(&stream, 0, sizeof(stream));

        // Initialize deflate with raw deflate mode (no zlib header/trailer)
        if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                         -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            spdlog::error("Failed to initialize compression");
            return data;
        }

        stream.avail_in = static_cast<uInt>(data.size());
        stream.next_in = const_cast<uint8_t *>(data.data());

        std::vector<uint8_t> compressed;
        uint8_t out_buffer[8192];

        do
        {
            stream.avail_out = sizeof(out_buffer);
            stream.next_out = out_buffer;

            int ret = deflate(&stream, Z_FINISH);
            if (ret == Z_STREAM_ERROR)
            {
                spdlog::error("Failed to compress data");
                deflateEnd(&stream);
                return data;
            }

            size_t have = sizeof(out_buffer) - stream.avail_out;
            compressed.insert(compressed.end(), out_buffer, out_buffer + have);
        } while (stream.avail_out == 0);

        deflateEnd(&stream);
        return compressed;
    }

    /**
     * @brief 写入 ZIP 中央目录和中央目录结束记录
     *
     * 中央目录包含 ZIP 存档中所有文件的元数据，
     * 允许 ZIP 读取器高效地定位和提取文件。
     * 该方法写入中央目录和中央目录结束记录。
     */
    void WriteCentralDirectory()
    {
        uint32_t central_dir_offset = static_cast<uint32_t>(out_file_.tellp());

        // Write central directory entries for each file
        for (const auto &entry : central_dir_entries_)
        {
            ZipCentralDirectory dir;
            dir.signature = 0x02014b50;
            dir.version_made_by = 20;
            dir.version_needed = 20;
            dir.flags = 0;
            dir.compression = 8;
            dir.mod_time = 0;
            dir.mod_date = 0;
            dir.crc32 = entry.crc32;
            dir.compressed_size = entry.compressed_size;
            dir.uncompressed_size = entry.uncompressed_size;
            dir.filename_length = static_cast<uint16_t>(entry.relative_path.size());
            dir.extra_length = 0;
            dir.file_comment_length = 0;
            dir.disk_number_start = 0;
            dir.internal_file_attributes = 0;
            dir.external_file_attributes = 0x81000000; // Regular file attributes
            dir.relative_offset_local_header = entry.local_header_offset;

            out_file_.write(reinterpret_cast<const char *>(&dir), sizeof(dir));
            out_file_.write(entry.relative_path.c_str(), entry.relative_path.size());
        }

        uint32_t central_dir_size = static_cast<uint32_t>(out_file_.tellp()) - central_dir_offset;

        // Write end of central directory record
        ZipEndOfCentralDirectory end;
        end.signature = 0x06054b50;
        end.disk_number = 0;
        end.central_dir_disk = 0;
        end.entries_on_disk = static_cast<uint16_t>(central_dir_entries_.size());
        end.total_entries = static_cast<uint16_t>(central_dir_entries_.size());
        end.central_dir_size = central_dir_size;
        end.central_dir_offset = central_dir_offset;
        end.comment_length = 0;

        out_file_.write(reinterpret_cast<const char *>(&end), sizeof(end));
    }

private:
    struct CentralDirEntry
    {
        std::string relative_path;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint32_t local_header_offset;
    };

    std::ofstream out_file_;
    std::vector<CentralDirEntry> central_dir_entries_;
    std::vector<uint32_t> local_headers_;
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

        // Create some test files
        std::ofstream(test_dir + "/file1.txt") << "This is the content of the first test file\n";
        std::ofstream(test_dir + "/file2.txt") << "This is the content of the second test file\nHello World!\n";
        std::ofstream(test_dir + "/file3.txt") << "This is the third test file\nContains multiple lines\nThird line content\n";

        // Create subdirectory
        std::string subdir = test_dir + "/subfolder";
        fs::create_directories(subdir);
        std::ofstream(subdir + "/nested_file.txt") << "This is a file in the subfolder\n";

        return true;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Failed to create test directory: {}", e.what());
        return false;
    }
}

/**
 * @brief ZIP 压缩示例的主入口点
 *
 * 通过创建测试目录、将其压缩为 ZIP 文件并清理测试目录，
 * 演示 FileZipper 类的用法。
 *
 * @return 成功时返回 0，失败时返回 1
 */
int main(int, char *[])
{
    // 将 spdlog 配置为显示信息级别的消息
    spdlog::set_level(spdlog::level::info);

    // 创建测试目录
    std::string test_dir = "test_zip_source";
    std::string output_zip = "test_output.zip";

    spdlog::info("=== Zlib Directory Compression Example ===");

    // 创建测试目录和文件
    if (!CreateTestDirectory(test_dir))
    {
        spdlog::error("Failed to create test directory");
        return 1;
    }

    spdlog::info("Test directory created successfully: {}", test_dir);

    // 执行压缩
    FileZipper zipper;
    if (!zipper.CompressDirectory(test_dir, output_zip))
    {
        spdlog::error("Compression failed");
        return 1;
    }

    spdlog::info("Compression completed! You can verify using decompression tools: {}", output_zip);

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

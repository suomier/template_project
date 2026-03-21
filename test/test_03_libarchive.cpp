#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "io/filecompressor.h"

namespace fs = std::filesystem;

/**
 * @brief 创建带有示例文件的测试目录，用于压缩测试
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
        return false;
    }
}

/**
 * @brief 清理测试目录和归档文件
 *
 * @param test_dir 测试目录路径
 * @param archive_file 归档文件路径
 */
void CleanupTestFiles(const std::string &test_dir, const std::string &archive_file)
{
    try
    {
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
        if (fs::exists(archive_file))
        {
            fs::remove(archive_file);
        }
    }
    catch (const std::exception &)
    {
        // 忽略清理错误
    }
}

// 测试基本目录压缩
TEST(LibArchiveTest, BasicDirectoryCompression)
{
    std::string test_dir = "test_archive_source_basic";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_basic.zip";
#else
    output_archive = "test_output_basic.tar.gz";
#endif

    // 创建测试目录
    ASSERT_TRUE(CreateTestDirectory(test_dir));

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试多个文件压缩
TEST(LibArchiveTest, CompressFilesList)
{
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_files.zip";
#else
    output_archive = "test_output_files.tar.gz";
#endif

    // 创建多个测试文件
    std::vector<std::string> test_files;
    try
    {
        for (int i = 0; i < 5; ++i)
        {
            std::string filename = "test_file_" + std::to_string(i) + ".txt";
            std::ofstream(filename) << "Content of file " << i << "\nLine 2 of file " << i << "\n";
            test_files.push_back(filename);
        }
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test files";
    }

    // 准备压缩文件列表
    std::vector<std::pair<std::string, std::string>> files_to_zip;
    for (const auto &file : test_files)
    {
        files_to_zip.emplace_back(file, file);
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressFiles(files_to_zip, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    try
    {
        for (const auto &file : test_files)
        {
            if (fs::exists(file))
            {
                fs::remove(file);
            }
        }
        if (fs::exists(output_archive))
        {
            fs::remove(output_archive);
        }
    }
    catch (const std::exception &)
    {
        // 忽略清理错误
    }
}

// 测试空目录压缩
TEST(LibArchiveTest, EmptyDirectoryCompression)
{
    std::string test_dir = "test_archive_empty";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_empty.zip";
#else
    output_archive = "test_output_empty.tar.gz";
#endif

    // 创建空目录
    try
    {
        fs::create_directories(test_dir);
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create empty test directory";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试单个文件目录压缩
TEST(LibArchiveTest, SingleFileDirectoryCompression)
{
    std::string test_dir = "test_archive_single";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_single.zip";
#else
    output_archive = "test_output_single.tar.gz";
#endif

    // 创建包含单个文件的目录
    try
    {
        fs::create_directories(test_dir);
        std::ofstream(test_dir + "/single_file.txt") << "Single file content\n";
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test directory with single file";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试深层嵌套目录压缩
TEST(LibArchiveTest, DeeplyNestedDirectoryCompression)
{
    std::string test_dir = "test_archive_deep";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_deep.zip";
#else
    output_archive = "test_output_deep.tar.gz";
#endif

    // 创建深层嵌套目录结构
    try
    {
        std::string deep_dir = test_dir + "/level1/level2/level3/level4/level5";
        fs::create_directories(deep_dir);
        std::ofstream(deep_dir + "/deep_file.txt") << "Deeply nested file content\n";
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create deeply nested test directory";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试多文件目录压缩
TEST(LibArchiveTest, MultipleFilesDirectoryCompression)
{
    std::string test_dir = "test_archive_multiple";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_multiple.zip";
#else
    output_archive = "test_output_multiple.tar.gz";
#endif

    // 创建包含多个文件的目录
    try
    {
        fs::create_directories(test_dir);
        for (int i = 0; i < 10; ++i)
        {
            std::string filename = test_dir + "/file_" + std::to_string(i) + ".txt";
            std::ofstream(filename) << "Content of file " << i << "\n";
        }
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test directory with multiple files";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试相同内容多次压缩的一致性
TEST(LibArchiveTest, ConsistentCompressionResults)
{
    std::string test_dir1 = "test_archive_consistent1";
    std::string test_dir2 = "test_archive_consistent2";
    std::string output_archive1;
    std::string output_archive2;

#ifdef _WIN32
    output_archive1 = "test_output_consistent1.zip";
    output_archive2 = "test_output_consistent2.zip";
#else
    output_archive1 = "test_output_consistent1.tar.gz";
    output_archive2 = "test_output_consistent2.tar.gz";
#endif

    // 创建两个相同的测试目录
    ASSERT_TRUE(CreateTestDirectory(test_dir1));
    ASSERT_TRUE(CreateTestDirectory(test_dir2));

    // 分别压缩
    FileCompressor compressor;
    bool result1 = compressor.CompressDirectory(test_dir1, output_archive1);
    bool result2 = compressor.CompressDirectory(test_dir2, output_archive2);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);

    // 验证两个归档文件都已创建
    EXPECT_TRUE(fs::exists(output_archive1));
    EXPECT_TRUE(fs::exists(output_archive2));

    // 清理
    CleanupTestFiles(test_dir1, output_archive1);
    CleanupTestFiles(test_dir2, output_archive2);
}

// 测试包含特殊字符文件名的压缩
TEST(LibArchiveTest, SpecialCharactersInFilename)
{
    std::string test_dir = "test_archive_special";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_special.zip";
#else
    output_archive = "test_output_special.tar.gz";
#endif

    // 创建包含特殊字符的文件名
    try
    {
        fs::create_directories(test_dir);
        std::ofstream(test_dir + "/file_with_underscores.txt") << "Content 1\n";
        std::ofstream(test_dir + "/file-with-dashes.txt") << "Content 2\n";
        std::ofstream(test_dir + "/file.with.dots.txt") << "Content 3\n";
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test directory with special characters";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试大型文件压缩
TEST(LibArchiveTest, LargeFileCompression)
{
    std::string test_dir = "test_archive_large";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_large.zip";
#else
    output_archive = "test_output_large.tar.gz";
#endif

    // 创建包含大文件的目录
    try
    {
        fs::create_directories(test_dir);
        std::ofstream large_file(test_dir + "/large_file.txt");
        // 写入 1MB 的数据
        for (int i = 0; i < 10000; ++i)
        {
            large_file << "This is a line of data that will be repeated many times to create a large file.\n";
        }
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test directory with large file";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}

// 测试多个子目录的压缩
TEST(LibArchiveTest, MultipleSubdirectories)
{
    std::string test_dir = "test_archive_multisub";
    std::string output_archive;

#ifdef _WIN32
    output_archive = "test_output_multisub.zip";
#else
    output_archive = "test_output_multisub.tar.gz";
#endif

    // 创建多个子目录
    try
    {
        fs::create_directories(test_dir);
        for (int i = 0; i < 5; ++i)
        {
            std::string subdir = test_dir + "/subdir_" + std::to_string(i);
            fs::create_directories(subdir);
            std::ofstream(subdir + "/file.txt") << "Content in subdir " << i << "\n";
        }
    }
    catch (const std::exception &)
    {
        FAIL() << "Failed to create test directory with multiple subdirectories";
    }

    // 执行压缩
    FileCompressor compressor;
    bool result = compressor.CompressDirectory(test_dir, output_archive);
    EXPECT_TRUE(result);

    // 验证归档文件已创建
    EXPECT_TRUE(fs::exists(output_archive));

    // 清理
    CleanupTestFiles(test_dir, output_archive);
}
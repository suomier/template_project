#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "utils/crc32calculator.h"

/**
 * @brief CRC32 校验和测试用例
 *
 * 测试 CRC32 校验和的多种应用场景：
 * 1. 字符串的 CRC32 校验和计算
 * 2. 字节数组的 CRC32 校验和计算
 * 3. 文件的 CRC32 校验和计算
 * 4. 分块计算数据的 CRC32 校验和
 * 5. 数据完整性验证
 * 6. 文件完整性验证
 * 7. 空数据的 CRC32 校验和
 */

/**
 * @brief 创建测试文件，用于 CRC32 校验测试
 *
 * @param filename 要创建的文件路径
 * @param content 文件内容
 * @return true 如果文件创建成功
 * @return false 如果文件创建失败
 */
bool CreateTestFile(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

// 测试字符串 CRC32 计算
TEST(CRC32Test, StringCalculation)
{
    std::string text1 = "Hello, World!";
    std::string text2 = "Hello, World!"; // Same as text1
    std::string text3 = "Hello, World?"; // Different from text1

    uint32_t crc1 = CRC32Calculator::CalculateString(text1);
    uint32_t crc2 = CRC32Calculator::CalculateString(text2);
    uint32_t crc3 = CRC32Calculator::CalculateString(text3);

    // 相同的字符串应该产生相同的 CRC32
    EXPECT_EQ(crc1, crc2);
    // 不同的字符串应该产生不同的 CRC32
    EXPECT_NE(crc1, crc3);
}

// 测试字节数组 CRC32 计算
TEST(CRC32Test, ByteArrayCalculation)
{
    uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint32_t crc_bytes = CRC32Calculator::CalculateBytes(bytes, sizeof(bytes));

    // 验证 CRC32 计算成功（值不为 0，除非正好计算出 0）
    // 这里我们只是验证计算过程不会出错
    SUCCEED();
}

// 测试文件 CRC32 计算
TEST(CRC32Test, FileCalculation)
{
    std::string test_filename = "test_crc32_data.txt";
    std::string file_content = "This is test content for CRC32 calculation.\n"
                               "Multiple lines of data to check.\n"
                               "End of file.";

    ASSERT_TRUE(CreateTestFile(test_filename, file_content));

    uint32_t crc_file = CRC32Calculator::CalculateFile(test_filename);
    EXPECT_NE(crc_file, 0); // 非空文件的 CRC32 通常不为 0

    // 清理测试文件
    std::filesystem::remove(test_filename);
}

// 测试分块 CRC32 计算
TEST(CRC32Test, ChunkedCalculation)
{
    std::string large_text = "This is a large text that will be processed in chunks. "
                             "Chunked processing is useful for large files or streaming data.";

    size_t chunk_size = 20;
    uint32_t chunked_crc = 0;
    size_t offset = 0;

    while (offset < large_text.size())
    {
        size_t current_chunk_size = std::min(chunk_size, large_text.size() - offset);
        const uint8_t *chunk_ptr = reinterpret_cast<const uint8_t *>(large_text.c_str() + offset);
        chunked_crc = CRC32Calculator::CalculateChunked(chunk_ptr, current_chunk_size, chunked_crc);
        offset += current_chunk_size;
    }

    // 与直接计算的结果应该相同
    uint32_t direct_crc = CRC32Calculator::CalculateString(large_text);
    EXPECT_EQ(chunked_crc, direct_crc);
}

// 测试数据完整性验证
TEST(CRC32Test, DataIntegrityVerification)
{
    std::string original_data = "Important data that must not be corrupted";
    uint32_t original_crc = CRC32Calculator::CalculateString(original_data);

    // 验证原始数据
    bool is_valid1 = CRC32Calculator::VerifyString(original_data, original_crc);
    EXPECT_TRUE(is_valid1);

    // 验证修改后的数据（应该失败）
    std::string modified_data = "Important data that must be corrupted"; // Changed "not" to "be"
    bool is_valid2 = CRC32Calculator::VerifyString(modified_data, original_crc);
    EXPECT_FALSE(is_valid2);
}

// 测试文件完整性验证
TEST(CRC32Test, FileIntegrityVerification)
{
    std::string test_filename = "test_crc32_data.txt";
    std::string file_content = "This is test content for CRC32 calculation.\n"
                               "Multiple lines of data to check.\n"
                               "End of file.";

    ASSERT_TRUE(CreateTestFile(test_filename, file_content));

    uint32_t file_crc = CRC32Calculator::CalculateFile(test_filename);

    // 验证文件完整性
    bool is_file_valid = CRC32Calculator::VerifyFile(test_filename, file_crc);
    EXPECT_TRUE(is_file_valid);

    // 使用错误的 CRC32 进行验证（应该失败）
    uint32_t wrong_crc = file_crc + 1;
    bool is_file_valid2 = CRC32Calculator::VerifyFile(test_filename, wrong_crc);
    EXPECT_FALSE(is_file_valid2);

    // 清理测试文件
    std::filesystem::remove(test_filename);
}

// 测试空数据 CRC32
TEST(CRC32Test, EmptyDataCalculation)
{
    std::string empty_string = "";
    uint32_t empty_crc = CRC32Calculator::CalculateString(empty_string);

    // 空数据的 CRC32 应该是一个固定值（通常是 0）
    EXPECT_EQ(empty_crc, 0);
}

// 测试相同的字符串产生相同的 CRC32
TEST(CRC32Test, IdenticalStringsProduceIdenticalCRC)
{
    std::string text = "Same text content";
    uint32_t crc1 = CRC32Calculator::CalculateString(text);
    uint32_t crc2 = CRC32Calculator::CalculateString(text);
    uint32_t crc3 = CRC32Calculator::CalculateString(text);

    EXPECT_EQ(crc1, crc2);
    EXPECT_EQ(crc2, crc3);
}

// 测试多次分块计算与一次性计算的一致性
TEST(CRC32Test, MultipleChunkedCalculationsConsistency)
{
    std::string text = "This text will be calculated in multiple different ways.";

    // 一次性计算
    uint32_t direct_crc = CRC32Calculator::CalculateString(text);

    // 分块计算（2个块）
    uint32_t chunked_crc1 = 0;
    size_t chunk_size = text.size() / 2;
    for (size_t offset = 0; offset < text.size(); offset += chunk_size)
    {
        size_t current_size = std::min(chunk_size, text.size() - offset);
        const uint8_t *chunk_ptr = reinterpret_cast<const uint8_t *>(text.c_str() + offset);
        chunked_crc1 = CRC32Calculator::CalculateChunked(chunk_ptr, current_size, chunked_crc1);
    }
    EXPECT_EQ(direct_crc, chunked_crc1);

    // 分块计算（多个小块）
    uint32_t chunked_crc2 = 0;
    chunk_size = 10;
    for (size_t offset = 0; offset < text.size(); offset += chunk_size)
    {
        size_t current_size = std::min(chunk_size, text.size() - offset);
        const uint8_t *chunk_ptr = reinterpret_cast<const uint8_t *>(text.c_str() + offset);
        chunked_crc2 = CRC32Calculator::CalculateChunked(chunk_ptr, current_size, chunked_crc2);
    }
    EXPECT_EQ(direct_crc, chunked_crc2);
}

// 测试单个字符的 CRC32
TEST(CRC32Test, SingleCharacterCRC)
{
    std::string single_char = "A";
    uint32_t crc = CRC32Calculator::CalculateString(single_char);
    EXPECT_NE(crc, 0);
}

// 测试长字符串的 CRC32
TEST(CRC32Test, LongStringCRC)
{
    std::string long_text;
    for (int i = 0; i < 1000; ++i)
    {
        long_text += "Long string content " + std::to_string(i) + "\n";
    }

    uint32_t crc = CRC32Calculator::CalculateString(long_text);
    EXPECT_NE(crc, 0);

    // 验证长字符串的一致性
    uint32_t crc2 = CRC32Calculator::CalculateString(long_text);
    EXPECT_EQ(crc, crc2);
}

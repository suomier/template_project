#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <zlib.h>

/**
 * @brief CRC32 校验和计算工具类
 *
 * 该类提供计算 CRC32 校验和的功能，用于验证数据的完整性。
 * CRC32（循环冗余校验）是一种常用的数据校验算法，能够检测数据传输或存储过程中的错误。
 */
class CRC32Calculator
{
public:
    /**
     * @brief 计算字符串的 CRC32 校验和
     *
     * @param data 要计算校验和的字符串数据
     * @return uint32_t 计算得到的 32 位 CRC32 校验和
     */
    static uint32_t CalculateString(const std::string &data)
    {
        return crc32(0, reinterpret_cast<const Bytef *>(data.c_str()), data.size());
    }

    /**
     * @brief 计算字节数组的 CRC32 校验和
     *
     * @param data 指向数据数组的指针
     * @param size 数据的字节大小
     * @return uint32_t 计算得到的 32 位 CRC32 校验和
     */
    static uint32_t CalculateBytes(const uint8_t *data, size_t size)
    {
        return crc32(0, data, size);
    }

    /**
     * @brief 计算文件内容的 CRC32 校验和
     *
     * 该方法读取指定文件的全部内容，并计算其 CRC32 校验和。
     * 文件以二进制模式打开，确保能够正确处理所有类型的数据。
     *
     * @param filename 要计算校验和的文件路径
     * @return uint32_t 计算得到的 32 位 CRC32 校验和，如果文件读取失败则返回 0
     */
    static uint32_t CalculateFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            spdlog::error("Failed to open file: {}", filename);
            return 0;
        }

        // 读取文件内容到缓冲区
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (buffer.empty())
        {
            spdlog::warn("File is empty: {}", filename);
            return crc32(0, Z_NULL, 0);
        }

        // 计算 CRC32 校验和
        uint32_t crc = crc32(0, buffer.data(), buffer.size());
        return crc;
    }

    /**
     * @brief 分块计算数据的 CRC32 校验和
     *
     * 该方法适用于大文件或流式数据，通过分块处理避免一次性加载所有数据到内存。
     * 每次调用都会更新 CRC32 值，初始值为 0，后续调用使用前一次的计算结果。
     *
     * @param data 数据块指针
     * @param size 数据块大小
     * @param previous_crc 前一次计算的 CRC32 值，首次调用时使用 0
     * @return uint32_t 更新后的 CRC32 校验和
     */
    static uint32_t CalculateChunked(const uint8_t *data, size_t size, uint32_t previous_crc = 0)
    {
        return crc32(previous_crc, data, size);
    }

    /**
     * @brief 验证字符串的完整性
     *
     * 通过计算字符串的 CRC32 校验和并与预期的校验和比较，验证数据是否损坏或被篡改。
     *
     * @param data 要验证的字符串数据
     * @param expected_crc 预期的 CRC32 校验和
     * @return true 如果校验和匹配，数据完整
     * @return false 如果校验和不匹配，数据可能损坏
     */
    static bool VerifyString(const std::string &data, uint32_t expected_crc)
    {
        uint32_t calculated_crc = CalculateString(data);
        return calculated_crc == expected_crc;
    }

    /**
     * @brief 验证文件的完整性
     *
     * 通过计算文件的 CRC32 校验和并与预期的校验和比较，验证文件是否损坏或被篡改。
     *
     * @param filename 要验证的文件路径
     * @param expected_crc 预期的 CRC32 校验和
     * @return true 如果校验和匹配，文件完整
     * @return false 如果校验和不匹配或文件读取失败
     */
    static bool VerifyFile(const std::string &filename, uint32_t expected_crc)
    {
        uint32_t calculated_crc = CalculateFile(filename);
        if (calculated_crc == 0 && expected_crc != 0)
        {
            return false; // 文件读取失败
        }
        return calculated_crc == expected_crc;
    }

    /**
     * @brief 将 CRC32 值格式化为十六进制字符串
     *
     * @param crc CRC32 校验和
     * @return std::string 格式化的十六进制字符串，前缀为 "0x"
     */
    static std::string FormatHex(uint32_t crc)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "0x%08X", crc);
        return std::string(buffer);
    }
};

/**
 * @brief 创建测试文件，用于 CRC32 校验测试
 *
 * 该函数创建一个包含指定内容的文本文件，用于演示 CRC32 校验功能。
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
        spdlog::error("Failed to create test file: {}", filename);
        return false;
    }

    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

/**
 * @brief CRC32 校验和示例的主入口点
 *
 * 该示例演示了 CRC32 校验和的多种应用场景：
 * 1. 计算字符串的 CRC32 校验和
 * 2. 计算文件的 CRC32 校验和
 * 3. 分块计算数据的 CRC32 校验和
 * 4. 验证数据完整性
 * 5. 比较不同数据的校验和
 *
 * @return 成功时返回 0，失败时返回 1
 */
int main(int, char *[])
{
    // Configure spdlog to display info level messages
    spdlog::set_level(spdlog::level::info);

    spdlog::info("=== CRC32 Checksum Calculation and Verification Example ===");

    // ========================================================================
    // Example 1: Calculate CRC32 for strings
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 1: String CRC32 Calculation ---");

    std::string text1 = "Hello, World!";
    std::string text2 = "Hello, World!"; // Same as text1
    std::string text3 = "Hello, World?"; // Different from text1

    uint32_t crc1 = CRC32Calculator::CalculateString(text1);
    uint32_t crc2 = CRC32Calculator::CalculateString(text2);
    uint32_t crc3 = CRC32Calculator::CalculateString(text3);

    spdlog::info("Text: \"{}\"", text1);
    spdlog::info("CRC32: {}", CRC32Calculator::FormatHex(crc1));
    spdlog::info("");

    spdlog::info("Text: \"{}\"", text2);
    spdlog::info("CRC32: {}", CRC32Calculator::FormatHex(crc2));
    spdlog::info("");

    spdlog::info("Text: \"{}\"", text3);
    spdlog::info("CRC32: {}", CRC32Calculator::FormatHex(crc3));
    spdlog::info("");

    spdlog::info("CRC32(text1) == CRC32(text2): {}", crc1 == crc2);
    spdlog::info("CRC32(text1) == CRC32(text3): {}", crc1 == crc3);

    // ========================================================================
    // Example 2: Calculate CRC32 for byte arrays
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 2: Byte Array CRC32 Calculation ---");

    uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint32_t crc_bytes = CRC32Calculator::CalculateBytes(bytes, sizeof(bytes));

    spdlog::info("Byte array: {{0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, "
                 "0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}}}",
                 bytes[0], bytes[1], bytes[2], bytes[3],
                 bytes[4], bytes[5], bytes[6], bytes[7]);
    spdlog::info("CRC32: {}", CRC32Calculator::FormatHex(crc_bytes));

    // ========================================================================
    // Example 3: Calculate CRC32 for files
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 3: File CRC32 Calculation ---");

    std::string test_filename = "test_crc32_data.txt";
    std::string file_content = "This is test content for CRC32 calculation.\n"
                               "Multiple lines of data to check.\n"
                               "End of file.";

    if (CreateTestFile(test_filename, file_content))
    {
        uint32_t crc_file = CRC32Calculator::CalculateFile(test_filename);
        spdlog::info("File: {}", test_filename);
        spdlog::info("Content length: {} bytes", file_content.size());
        spdlog::info("CRC32: {}", CRC32Calculator::FormatHex(crc_file));
    }

    // ========================================================================
    // Example 4: Chunked CRC32 calculation for large data
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 4: Chunked CRC32 Calculation ---");

    std::string large_text = "This is a large text that will be processed in chunks. "
                             "Chunked processing is useful for large files or streaming data.";

    size_t chunk_size = 20;
    uint32_t chunked_crc = 0;
    size_t offset = 0;

    spdlog::info("Text: \"{}\"", large_text);
    spdlog::info("Processing in chunks of {} bytes...", chunk_size);

    while (offset < large_text.size())
    {
        size_t current_chunk_size = std::min(chunk_size, large_text.size() - offset);
        const uint8_t *chunk_ptr = reinterpret_cast<const uint8_t *>(large_text.c_str() + offset);
        chunked_crc = CRC32Calculator::CalculateChunked(chunk_ptr, current_chunk_size, chunked_crc);

        spdlog::info("Chunk [{}, {}]: CRC32 so far = {}",
                     offset, offset + current_chunk_size - 1,
                     CRC32Calculator::FormatHex(chunked_crc));

        offset += current_chunk_size;
    }

    // Compare with direct calculation
    uint32_t direct_crc = CRC32Calculator::CalculateString(large_text);
    spdlog::info("");
    spdlog::info("Final chunked CRC32: {}", CRC32Calculator::FormatHex(chunked_crc));
    spdlog::info("Direct CRC32: {}", CRC32Calculator::FormatHex(direct_crc));
    spdlog::info("Results match: {}", chunked_crc == direct_crc);

    // ========================================================================
    // Example 5: Data integrity verification
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 5: Data Integrity Verification ---");

    std::string original_data = "Important data that must not be corrupted";
    uint32_t original_crc = CRC32Calculator::CalculateString(original_data);

    spdlog::info("Original data: \"{}\"", original_data);
    spdlog::info("Original CRC32: {}", CRC32Calculator::FormatHex(original_crc));
    spdlog::info("");

    // Verify original data
    bool is_valid1 = CRC32Calculator::VerifyString(original_data, original_crc);
    spdlog::info("Verify original data: {}", is_valid1 ? "PASS" : "FAIL");

    // Verify modified data (should fail)
    std::string modified_data = "Important data that must be corrupted"; // Changed "not" to "be"
    bool is_valid2 = CRC32Calculator::VerifyString(modified_data, original_crc);
    spdlog::info("Verify modified data: {}", is_valid2 ? "PASS" : "FAIL");

    // ========================================================================
    // Example 6: File integrity verification
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 6: File Integrity Verification ---");

    if (CreateTestFile(test_filename, file_content))
    {
        uint32_t file_crc = CRC32Calculator::CalculateFile(test_filename);
        spdlog::info("File CRC32 calculated: {}", CRC32Calculator::FormatHex(file_crc));

        // Verify file integrity
        bool is_file_valid = CRC32Calculator::VerifyFile(test_filename, file_crc);
        spdlog::info("File verification: {}", is_file_valid ? "PASS" : "FAIL");

        // Try to verify with wrong CRC (should fail)
        uint32_t wrong_crc = file_crc + 1;
        bool is_file_valid2 = CRC32Calculator::VerifyFile(test_filename, wrong_crc);
        spdlog::info("File verification with wrong CRC: {}", is_file_valid2 ? "PASS" : "FAIL");
    }

    // ========================================================================
    // Example 7: CRC32 of empty data
    // ========================================================================
    spdlog::info("");
    spdlog::info("--- Example 7: Empty Data CRC32 ---");

    std::string empty_string = "";
    uint32_t empty_crc = CRC32Calculator::CalculateString(empty_string);
    spdlog::info("Empty string CRC32: {}", CRC32Calculator::FormatHex(empty_crc));
    spdlog::info("Note: CRC32 of empty data is always {}", CRC32Calculator::FormatHex(empty_crc));

    // ========================================================================
    // Cleanup
    // ========================================================================
    spdlog::info("");
    spdlog::info("=== Cleaning up test files ===");
    try
    {
        std::filesystem::remove(test_filename);
        spdlog::info("Test file deleted: {}", test_filename);
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Failed to delete test file: {}", e.what());
    }

    spdlog::info("");
    spdlog::info("=== CRC32 Example Completed Successfully ===");
    return 0;
}

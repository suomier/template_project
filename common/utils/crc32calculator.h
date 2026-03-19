#ifndef CRC32CALCULATOR_H
#define CRC32CALCULATOR_H

#include <cstdint>
#include <string>

/**
 * @brief CRC32 校验和计算工具类
 *
 * 该类提供计算 CRC32 校验和的功能，用于验证数据的完整性。
 * CRC32（循环冗余校验）是一种常用的数据校验算法，能够检测数据传输或存储过程中的错误。
 */
namespace CRC32Calculator
{

/**
 * @brief 计算字符串的 CRC32 校验和
 *
 * @param data 要计算校验和的字符串数据
 * @return uint32_t 计算得到的 32 位 CRC32 校验和
 */
uint32_t CalculateString(const std::string &data);

/**
 * @brief 计算字节数组的 CRC32 校验和
 *
 * @param data 指向数据数组的指针
 * @param size 数据的字节大小
 * @return uint32_t 计算得到的 32 位 CRC32 校验和
 */
uint32_t CalculateBytes(const uint8_t *data, size_t size);

/**
 * @brief 计算文件内容的 CRC32 校验和
 *
 * 该方法读取指定文件的全部内容，并计算其 CRC32 校验和。
 * 文件以二进制模式打开，确保能够正确处理所有类型的数据。
 *
 * @param filename 要计算校验和的文件路径
 * @return uint32_t 计算得到的 32 位 CRC32 校验和，如果文件读取失败则返回 0
 */
uint32_t CalculateFile(const std::string &filename);

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
uint32_t CalculateChunked(const uint8_t *data, size_t size, uint32_t previous_crc = 0);

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
bool VerifyString(const std::string &data, uint32_t expected_crc);

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
bool VerifyFile(const std::string &filename, uint32_t expected_crc);

/**
 * @brief 将 CRC32 值格式化为十六进制字符串
 *
 * @param crc CRC32 校验和
 * @return std::string 格式化的十六进制字符串，前缀为 "0x"
 */
std::string FormatHex(uint32_t crc);

}; // namespace CRC32Calculator

#endif // CRC32CALCULATOR_H

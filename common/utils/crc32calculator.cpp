#include "crc32calculator.h"

#include <filesystem>
#include <fstream>

#include <spdlog/spdlog.h>
#include <zlib.h>

uint32_t CRC32Calculator::CalculateString(const std::string &data)
{
    return crc32(0, reinterpret_cast<const Bytef *>(data.c_str()), data.size());
}

uint32_t CRC32Calculator::CalculateBytes(const uint8_t *data, size_t size)
{
    return crc32(0, data, size);
}

uint32_t CRC32Calculator::CalculateFile(const std::string &filename)
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

uint32_t CRC32Calculator::CalculateChunked(const uint8_t *data, size_t size, uint32_t previous_crc)
{
    return crc32(previous_crc, data, size);
}

bool CRC32Calculator::VerifyString(const std::string &data, uint32_t expected_crc)
{
    uint32_t calculated_crc = CalculateString(data);
    return calculated_crc == expected_crc;
}

bool CRC32Calculator::VerifyFile(const std::string &filename, uint32_t expected_crc)
{
    uint32_t calculated_crc = CalculateFile(filename);
    if (calculated_crc == 0 && expected_crc != 0)
    {
        return false; // 文件读取失败
    }
    return calculated_crc == expected_crc;
}

std::string CRC32Calculator::FormatHex(uint32_t crc)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "0x%08X", crc);
    return std::string(buffer);
}

#ifndef NET_UTILS_H
#define NET_UTILS_H

#include "network/network_def.h"

/**
 * @brief 根据IP和端口生成ioa_addr结构体
 * @param saddr IP地址字符串
 * @param port 端口号
 * @param addr 输出的ioa_addr结构体指针
 * @return 成功返回0，失败返回负值
 */
int make_ioa_addr(const uint8_t *saddr, int port, ioa_addr *addr);

/**
 * @brief 根据完整地址字符串生成ioa_addr结构体
 * @param saddr 地址字符串
 * @param default_port 默认端口号
 * @param addr 输出的ioa_addr结构体指针
 * @return 成功返回0，失败返回负值
 */
int make_ioa_addr_from_full_string(const uint8_t *saddr, int default_port, ioa_addr *addr);

///////////////////////// add hash //////////////////////////
/**
 * @brief 将地址转换为字符串（含端口）
 * @param addr 地址结构体指针
 * @param saddr 输出字符串缓冲区
 * @return 字符串长度
 */
int addr_to_string(const ioa_addr *addr, uint8_t *saddr);

/**
 * @brief 将地址转换为字符串（不含端口）
 * @param addr 地址结构体指针
 * @param saddr 输出字符串缓冲区
 * @return 字符串长度
 */
int addr_to_string_no_port(const ioa_addr *addr, uint8_t *saddr);

///////////////////////// add hash //////////////////////////

/**
 * @brief 计算32位整数的哈希值
 * @param a 输入整数
 * @return 哈希值
 */
uint32_t hash_int32(uint32_t a);

/**
 * @brief 计算64位整数的哈希值
 * @param a 输入整数
 * @return 哈希值
 */
uint64_t hash_int64(uint64_t a);

/**
 * @brief 计算地址的哈希值（含端口）
 * @param addr 地址结构体指针
 * @return 哈希值
 */
uint32_t addr_hash(const ioa_addr *addr);

/**
 * @brief 计算地址的哈希值（不含端口）
 * @param addr 地址结构体指针
 * @return 哈希值
 */
uint32_t addr_hash_no_port(const ioa_addr *addr);

/////// Check whether this is a good address //////////////
/**
 * @brief 判断地址是否为多播地址
 * @param a 地址结构体指针
 * @return 是返回true，否则返回false
 */
bool ioa_addr_is_multicast(ioa_addr *a);

/**
 * @brief 判断地址是否为回环地址
 * @param addr 地址结构体指针
 * @return 是返回true，否则返回false
 */
bool ioa_addr_is_loopback(ioa_addr *addr);

/**
 * @brief 判断地址是否为全零地址
 * @param addr 地址结构体指针
 * @return 是返回true，否则返回false
 */
bool ioa_addr_is_zero(ioa_addr *addr);

/**
 * @brief 判断地址是否为链接本地地址
 * @param addr 地址结构体指针
 * @return 是返回true，否则返回false
 */
bool ioa_addr_is_link_local(ioa_addr *addr);

/**
 * @brief 判断地址是否为本地地址（包括回环和链路本地）
 * @param addr 地址结构体指针
 * @return 是返回true，否则返回false
 */
bool ioa_addr_is_local(ioa_addr *addr);

/**
 * @brief 获取地址族名称
 * @param family 地址族（AF_INET/AF_INET6）
 * @return 地址族名称字符串
 */
const char *addr_family_to_str(int family);

///////////////////////// add copy //////////////////////////

/**
 * @brief 拷贝地址
 * @param dst 目标地址结构体指针
 * @param src 源地址结构体指针
 */
void addr_cpy(ioa_addr *dst, const ioa_addr *src);

/**
 * @brief 从IPv4 sockaddr拷贝到ioa_addr
 * @param dst 目标地址结构体指针
 * @param src 源IPv4 sockaddr指针
 */
void addr_cpy4(ioa_addr *dst, const struct sockaddr_in *src);

/**
 * @brief 从IPv6 sockaddr拷贝到ioa_addr
 * @param dst 目标地址结构体指针
 * @param src 源IPv6 sockaddr指针
 */
void addr_cpy6(ioa_addr *dst, const struct sockaddr_in6 *src);

/**
 * @brief 判断两个地址是否相等（含端口）
 * @param a1 地址1指针
 * @param a2 地址2指针
 * @return 相等返回true，否则返回false
 */
bool addr_eq(const ioa_addr *a1, const ioa_addr *a2);

/**
 * @brief 判断两个地址是否相等（不含端口）
 * @param a1 地址1指针
 * @param a2 地址2指针
 * @return 相等返回true，否则返回false
 */
bool addr_eq_no_port(const ioa_addr *a1, const ioa_addr *a2);

#endif // NET_UTILS_H

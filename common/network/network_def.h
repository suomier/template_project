#ifndef NETWORK_DEF_H
#define NETWORK_DEF_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
/* Windows 平台：需要包含 Windows Socket 头文件 */
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

/* Windows 平台特定的类型定义 */
typedef int socklen_t;

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef _w64 int ssize_t;
#endif

#else
/* Unix/Linux 平台：使用标准 BSD Socket 头文件 */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// socket_fd 跨平台定义
#ifdef _WIN32
#define socket_t intptr_t
#else
#define socket_t int
#endif

/**
 * @brief 通用地址联合体
 * @details 支持IPv4、IPv6和通用sockaddr结构的地址表示
 */
typedef union
{
    struct sockaddr ss;     /**< 通用套接字地址结构 */
    struct sockaddr_in s4;  /**< IPv4地址结构 */
    struct sockaddr_in6 s6; /**< IPv6地址结构 */
} ioa_addr;

/**
 * @brief 套接字类型枚举
 */
enum _SOCKET_TYPE
{
    UNKNOWN_SOCKET = 0,          ///< 未知套接字
    TCP_SOCKET = 6,              ///< TCP套接字
    UDP_SOCKET = 17,             ///< UDP套接字
    TLS_SOCKET = 56,             ///< TLS套接字
    SCTP_SOCKET = 132,           ///< SCTP套接字
    TLS_SCTP_SOCKET = 133,       ///< TLS over SCTP套接字
    DTLS_SOCKET = 250,           ///< DTLS套接字
    TCP_SOCKET_PROXY = 253,      ///< TCP代理套接字
    TENTATIVE_SCTP_SOCKET = 254, ///< 临时SCTP套接字
    TENTATIVE_TCP_SOCKET = 255   ///< 临时TCP套接字
};
typedef enum _SOCKET_TYPE SOCKET_TYPE;

/**
 * @brief 64位网络字节序转主机字节序
 * @param ull 64位网络字节序数值
 * @return 主机字节序数值
 */
static inline uint64_t ioa_ntoh64(uint64_t net64)
{
#ifdef _WIN32
    return _byteswap_uint64(net64);
#else
    return be64toh(net64);
#endif
}

/**
 * @brief 64位主机字节序转网络字节序
 * @param ull 64位主机字节序数值
 * @return 网络字节序数值
 */
static inline uint64_t ioa_hton64(uint64_t host64)
{
#ifdef _WIN32
    return _byteswap_uint64(host64);
#else
    return htobe64(host64);
#endif
}

/**
 * @brief 16位网络字节序转主机字节序
 * @param s 16位网络字节序数值
 * @return 主机字节序数值
 */
#define nswap16(s) ntohs(s)

/**
 * @brief 32位网络字节序转主机字节序
 * @param ul 32位网络字节序数值
 * @return 主机字节序数值
 */
#define nswap32(ul) ntohl(ul)

/**
 * @brief 64位网络字节序转主机字节序
 * @param ull 64位网络字节序数值
 * @return 主机字节序数值
 */
#define nswap64(ull) ioa_ntoh64(ull)

#endif // NETWORK_DEF_H

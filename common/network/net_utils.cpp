#include "net_utils.h"

#include "network/socket.h"
#include "utils/string_utils.h"

/** @brief 最大地址字符串长度 */
#define MAX_IOA_ADDR_STRING (65)

/**
 * @brief 从地址字符串中分离地址和端口信息
 *
 * 该函数解析包含地址和端口的字符串，支持 IPv4 和 IPv6 两种格式。
 * IPv6 地址使用方括号包裹，例如 "[2001:db8::1]:3478"；
 * IPv4 地址直接使用冒号分隔，例如 "192.168.1.1:3478"。
 *
 * @details
 * 支持的格式：
 * - IPv4 带端口：\c "192.168.1.1:3478"
 * - IPv4 不带端口：\c "192.168.1.1"
 * - IPv6 带端口：\c "[2001:db8::1]:3478"
 * - IPv6 不带端口：\c "[2001:db8::1]"
 * - 带前导空格的格式也会被正确处理
 *
 * 解析过程：
 * 1. 跳过字符串前导空格
 * 2. 如果以 '[' 开头，则按 IPv6 格式解析
 * 3. 否则，按 IPv4 格式解析
 * 4. 提取端口号（如果存在）
 * 5. 返回地址字符串的起始指针
 *
 * @note
 * - 该函数会修改原始字符串（插入 '\0' 终止符）
 * - 返回的地址字符串指针指向原始字符串的内部
 * - 调用者不应释放返回的指针
 * - 端口号必须是有效的十进制数字
 *
 * @param s0 输入的地址字符串，可能包含端口信息
 * @param port 输出参数，用于返回解析出的端口号（如果不存在则为 0）
 *
 * @return 成功时返回指向地址字符串的指针；失败时返回 \c NULL
 *
 * @retval 非 NULL 指向地址字符串（以 '\0' 终止）
 * @retval NULL 解析失败
 */
static char *get_addr_string_and_port(char *s0, int *port)
{
    char *s = s0;
    /* ========== 跳过前导空格 ========== */
    /* 移除字符串开头的空格字符
     * 允许输入字符串包含前导空格，提高容错性
     */
    while (*s && (*s == ' '))
    {
        ++s;
    }

    /* ========== IPv6 地址格式解析 ========== */
    /* IPv6 地址使用方括号包裹："[address]:port"
     * 方括号用于区分地址中的冒号和端口号前的冒号
     */
    if (*s == '[')
    {
        /* 跳过开头的 '[' */
        ++s;

        /* 查找结尾的 ']' */
        char *tail = strstr(s, "]");
        if (tail)
        {
            /* 在 ']' 位置插入 '\0'，将地址字符串与端口部分分离
             * 这样 s 就指向以 '\0' 结尾的地址字符串
             */
            *tail = 0;
            ++tail;

            /* 跳过 ']' 后的空格 */
            while (*tail && (*tail == ' '))
            {
                ++tail;
            }

            /* 检查是否有端口号 */
            if (*tail == ':')
            {
                /* 有端口号，解析端口号
                 * 格式："] :3478"
                 * 跳过 ':'，然后使用 atoi 转换为整数
                 */
                ++tail;
                *port = atoi(tail);
                return s;
            }
            else if (*tail == 0)
            {
                /* 没有端口号，格式为 "[address]"
                 * 端口号设为 0
                 */
                *port = 0;
                return s;
            }
        }
    }
    /* ========== IPv4 地址格式解析 ========== */
    else
    {
        /* IPv4 地址直接使用冒号分隔："address:port"
         * 查找第一个冒号，它用于分隔地址和端口
         */
        char *tail = strstr(s, ":");
        if (tail)
        {
            /* 在 ':' 位置插入 '\0'，将地址字符串与端口部分分离 */
            *tail = 0;
            ++tail;

            /* 解析端口号 */
            *port = atoi(tail);
            return s;
        }
        else
        {
            /* 没有端口号，格式为 "address" */
            *port = 0;
            return s;
        }
    }

    /* 解析失败 */
    return NULL;
}

int make_ioa_addr(const uint8_t *saddr0, int port, ioa_addr *addr)
{
    if (!saddr0 || !addr)
    {
        return -1;
    }

    char ssaddr[257];
    STRCPY(ssaddr, saddr0);

    char *saddr = ssaddr;
    while (*saddr == ' ')
    {
        ++saddr;
    }

    size_t len = strlen(saddr);
    while (len > 0)
    {
        if (saddr[len - 1] == ' ')
        {
            saddr[len - 1] = 0;
            --len;
        }
        else
        {
            break;
        }
    }

    memset(addr, 0, sizeof(ioa_addr));
    if ((len == 0) || (inet_pton(AF_INET, saddr, &addr->s4.sin_addr) == 1))
    {
        addr->s4.sin_family = AF_INET;
#if defined(TURN_HAS_SIN_LEN) /* tested when configured */
        addr->s4.sin_len = sizeof(struct sockaddr_in);
#endif
        addr->s4.sin_port = nswap16(port);
    }
    else if (inet_pton(AF_INET6, saddr, &addr->s6.sin6_addr) == 1)
    {
        addr->s6.sin6_family = AF_INET6;
#if defined(SIN6_LEN) /* this define is required by IPv6 if used */
        addr->s6.sin6_len = sizeof(struct sockaddr_in6);
#endif
        addr->s6.sin6_port = nswap16(port);
    }
    else
    {
        struct addrinfo addr_hints;
        struct addrinfo *addr_result = NULL;
        int err;

        memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        addr_hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
        addr_hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
        addr_hints.ai_protocol = 0;          /* Any protocol */
        addr_hints.ai_canonname = NULL;
        addr_hints.ai_addr = NULL;
        addr_hints.ai_next = NULL;

        err = getaddrinfo(saddr, NULL, &addr_hints, &addr_result);
        if ((err != 0) || (!addr_result))
        {
            fprintf(stderr, "error resolving '%s' hostname: %s\n", saddr, gai_strerror(err));
            return -1;
        }

        int family = AF_INET;
        struct addrinfo *addr_result_orig = addr_result;
        int found = 0;

    beg_af:

        while (addr_result)
        {

            if (addr_result->ai_family == family)
            {
                if (addr_result->ai_family == AF_INET)
                {
                    memcpy(addr, addr_result->ai_addr, addr_result->ai_addrlen);
                    addr->s4.sin_port = nswap16(port);
#if defined(TURN_HAS_SIN_LEN) /* tested when configured */
                    addr->s4.sin_len = sizeof(struct sockaddr_in);
#endif
                    found = 1;
                    break;
                }
                else if (addr_result->ai_family == AF_INET6)
                {
                    memcpy(addr, addr_result->ai_addr, addr_result->ai_addrlen);
                    addr->s6.sin6_port = nswap16(port);
#if defined(SIN6_LEN) /* this define is required by IPv6 if used */
                    addr->s6.sin6_len = sizeof(struct sockaddr_in6);
#endif
                    found = 1;
                    break;
                }
            }

            addr_result = addr_result->ai_next;
        }

        if (!found && family == AF_INET)
        {
            family = AF_INET6;
            addr_result = addr_result_orig;
            goto beg_af;
        }

        freeaddrinfo(addr_result_orig);
    }

    return 0;
}

int make_ioa_addr_from_full_string(const uint8_t *saddr, int default_port, ioa_addr *addr)
{
    if (!addr || !saddr)
    {
        return -1;
    }

    char *s = strdup((const char *)saddr);
    if (!s)
    {
        return -1; // 检查 strdup 失败
    }

    int ret = -1;
    int port = 0;
    char *sa = get_addr_string_and_port(s, &port);
    if (sa)
    {
        if (port < 1)
        {
            port = default_port;
        }
        ret = make_ioa_addr((uint8_t *)sa, port, addr);
    }
    free(s);
    return ret;
}

int addr_to_string(const ioa_addr *addr, uint8_t *saddr)
{
    /* ========== 参数有效性检查 ========== */
    /* 检查地址指针和输出缓冲区指针是否有效
     * addr 不能为 NULL，否则无法获取地址信息
     * saddr 不能为 NULL，否则无法存储结果
     */
    if (addr && saddr)
    {
        /* 临时缓冲区，用于存储 inet_ntop() 转换后的地址字符串
         * INET6_ADDRSTRLEN: IPv6 地址字符串的最大长度（46 字节）
         * 也足够容纳 IPv4 地址（INET_ADDRSTRLEN = 16 字节）
         */
        char addrtmp[INET6_ADDRSTRLEN];

        /* ========== IPv4 地址处理 ========== */
        if (addr->ss.sa_family == AF_INET)
        {
            /* AF_INET: Address Family Internet（IPv4）
             * 使用 inet_ntop 将 IPv4 地址（4 字节）转换为字符串格式
             * addr->s4.sin_addr: 包含 IPv4 地址的 sockaddr_in 结构成员
             * INET_ADDRSTRLEN: IPv4 地址字符串的最大长度（16 字节）
             * 结果格式：点分十进制，如 "192.168.1.1"
             */
            inet_ntop(AF_INET, &addr->s4.sin_addr, addrtmp, INET_ADDRSTRLEN);

            /* 检查是否有端口号
             * 端口号大于 0 表示包含有效的端口信息
             * 格式化为 "address:port" 形式
             * 例如："192.168.1.1:3478"
             */
            if (addr_get_port(addr) > 0)
            {
                snprintf((char *)saddr, MAX_IOA_ADDR_STRING, "%s:%d", addrtmp, addr_get_port(addr));
            }
            else
            {
                /* 没有端口号，只格式化地址部分
                 * 例如："192.168.1.1"
                 */
                strncpy((char *)saddr, addrtmp, MAX_IOA_ADDR_STRING);
            }
        }
        /* ========== IPv6 地址处理 ========== */
        else if (addr->ss.sa_family == AF_INET6)
        {
            /* AF_INET6: Address Family Internet version 6
             * 使用 inet_ntop 将 IPv6 地址（16 字节）转换为字符串格式
             * addr->s6.sin6_addr: 包含 IPv6 地址的 sockaddr_in6 结构成员
             * INET6_ADDRSTRLEN: IPv6 地址字符串的最大长度（46 字节）
             * 结果格式：冒号分隔的十六进制，如 "2001:0db8:0000:0000:0000:ff00:0042:8329"
             * inet_ntop 会自动压缩连续的零，如 "2001:db8::ff00:42:8329"
             */
            inet_ntop(AF_INET6, &addr->s6.sin6_addr, addrtmp, INET6_ADDRSTRLEN);

            /* 检查是否有端口号
             * IPv6 地址使用方括号 [] 包裹地址部分
             * 这是为了区分地址中的冒号和端口号前的冒号
             * 格式化为 "[address]:port" 形式
             * 例如："[2001:db8::1]:3478"
             *
             * 为什么需要方括号：
             *   - IPv6 地址包含冒号（:）
             *   - 如果不使用方括号，"2001:db8::1:3478" 会产生歧义
             *   - 方括号明确标识地址部分，最后的冒号和数字是端口号
             */
            if (addr_get_port(addr) > 0)
            {
                snprintf((char *)saddr, MAX_IOA_ADDR_STRING, "[%s]:%d", addrtmp, addr_get_port(addr));
            }
            else
            {
                /* 没有端口号，只格式化地址部分
                 * 例如："2001:db8::1"
                 */
                strncpy((char *)saddr, addrtmp, MAX_IOA_ADDR_STRING);
            }
        }
        /* ========== 不支持的地址族 ========== */
        else
        {
            /* 既不是 IPv4 也不是 IPv6，无法处理
             * 直接返回错误
             */
            return -1;
        }

        /* 转换成功 */
        return 0;
    }

    /* 参数无效 */
    return -1;
}

int addr_to_string_no_port(const ioa_addr *addr, uint8_t *saddr)
{
    if (addr && saddr)
    {
        char addrtmp[MAX_IOA_ADDR_STRING];

        if (addr->ss.sa_family == AF_INET)
        {
            inet_ntop(AF_INET, &addr->s4.sin_addr, addrtmp, INET_ADDRSTRLEN);
            strncpy((char *)saddr, addrtmp, MAX_IOA_ADDR_STRING);
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            inet_ntop(AF_INET6, &addr->s6.sin6_addr, addrtmp, INET6_ADDRSTRLEN);
            strncpy((char *)saddr, addrtmp, MAX_IOA_ADDR_STRING);
        }
        else
        {
            return -1;
        }

        return 0;
    }

    return -1;
}

uint32_t hash_int32(uint32_t a)
{
    a = a ^ (a >> 4);
    a = (a ^ 0xdeadbeef) + (a << 5);
    a = a ^ (a >> 11);
    return a;
}

uint64_t hash_int64(uint64_t a)
{
    a = a ^ (a >> 4);
    a = (a ^ 0xdeadbeefdeadbeefLL) + (a << 5);
    a = a ^ (a >> 11);
    return a;
}

uint32_t addr_hash(const ioa_addr *addr)
{
    if (!addr)
    {
        return 0;
    }

    uint32_t ret = 0;
    if (addr->ss.sa_family == AF_INET)
    {
        ret = hash_int32(addr->s4.sin_addr.s_addr + addr->s4.sin_port);
    }
    else
    {
        uint64_t a[2];
        memcpy(&a, &(addr->s6.sin6_addr), sizeof(a));
        ret = (uint32_t)((hash_int64(a[0]) << 3) + (hash_int64(a[1] + addr->s6.sin6_port)));
    }
    return ret;
}

uint32_t addr_hash_no_port(const ioa_addr *addr)
{
    if (!addr)
    {
        return 0;
    }

    uint32_t ret = 0;
    if (addr->ss.sa_family == AF_INET)
    {
        ret = hash_int32(addr->s4.sin_addr.s_addr);
    }
    else
    {
        uint64_t a[2];
        memcpy(&a, &(addr->s6.sin6_addr), sizeof(a));
        ret = (uint32_t)((hash_int64(a[0]) << 3) + (hash_int64(a[1])));
    }
    return ret;
}

bool ioa_addr_is_multicast(ioa_addr *addr)
{
    if (addr)
    {
        if (addr->ss.sa_family == AF_INET)
        {
            const uint8_t *u = ((const uint8_t *)&(addr->s4.sin_addr));
            return (u[0] > 223);
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            uint8_t u = ((const uint8_t *)&(addr->s6.sin6_addr))[0];
            return (u == 255);
        }
    }
    return false;
}

bool ioa_addr_is_loopback(ioa_addr *addr)
{
    if (addr)
    {
        if (addr->ss.sa_family == AF_INET)
        {
            const uint8_t *u = ((const uint8_t *)&(addr->s4.sin_addr));
            return (u[0] == 127);
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            const uint8_t *u = ((const uint8_t *)&(addr->s6.sin6_addr));
            if (u[15] == 1)
            {
                int i;
                for (i = 0; i < 15; ++i)
                {
                    if (u[i])
                    {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

/*
为避免漏洞，此函数检查地址是否位于 0.0.0.0/8 或 ::/128 范围内。
源来自于 (INADDR_ANY) 0.0.0.0/32 和 (in6addr_any) ::/128，在 Linux 系统上为了兼容旧 BSD 而被路由到回环地址。
https://github.com/torvalds/linux/blob/a2f5ea9e314ba6778f885c805c921e9362ec0420/net/ipv6/tcp_ipv6.c#L182
为避免任何问题，我们匹配整个 0.0.0.0/8，该网段在 RFC6890 中被定义为本地网络"this"。
*/
bool ioa_addr_is_zero(ioa_addr *addr)
{
    if (addr)
    {
        if (addr->ss.sa_family == AF_INET)
        {
            const uint8_t *u = ((const uint8_t *)&(addr->s4.sin_addr));
            return (u[0] == 0);
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            const uint8_t *u = ((const uint8_t *)&(addr->s6.sin6_addr));
            int i;
            for (i = 0; i <= 15; ++i)
            {
                if (u[i])
                {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool ioa_addr_is_link_local(ioa_addr *addr)
{
    if (addr)
    {
        if (addr->ss.sa_family == AF_INET)
        {
            /* IPv4 链路本地地址: 169.254.0.0/16 */
            const uint8_t *u = ((const uint8_t *)&(addr->s4.sin_addr));
            return (u[0] == 169 && u[1] == 254);
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            /* IPv6 链路本地地址: fe80::/10 */
            const uint8_t *u = ((const uint8_t *)&(addr->s6.sin6_addr));
            return ((u[0] & 0xff) == 0xfe && (u[1] & 0xc0) == 0x80);
        }
    }
    return false;
}

bool ioa_addr_is_local(ioa_addr *addr)
{
    if (!addr)
    {
        return false;
    }
    /* 本地地址包括回环地址和链路本地地址 */
    return (ioa_addr_is_loopback(addr) || ioa_addr_is_link_local(addr));
}

const char *addr_family_to_str(int family)
{
    switch (family)
    {
    case AF_INET:
        return "AF_INET";
    case AF_INET6:
        return "AF_INET6";
    default:
        return "UNKNOWN";
    }
}

void addr_cpy(ioa_addr *dst, const ioa_addr *src)
{
    if (dst && src)
    {
        memcpy(dst, src, sizeof(ioa_addr));
    }
}

void addr_cpy4(ioa_addr *dst, const struct sockaddr_in *src)
{
    if (src && dst)
    {
        memcpy(dst, src, sizeof(struct sockaddr_in));
    }
}

void addr_cpy6(ioa_addr *dst, const struct sockaddr_in6 *src)
{
    if (src && dst)
    {
        memcpy(dst, src, sizeof(struct sockaddr_in6));
    }
}

bool addr_eq(const ioa_addr *a1, const ioa_addr *a2)
{
    if (!a1)
    {
        return (!a2);
    }
    else if (!a2)
    {
        return (!a1);
    }

    if (a1->ss.sa_family == a2->ss.sa_family)
    {
        if (a1->ss.sa_family == AF_INET && a1->s4.sin_port == a2->s4.sin_port)
        {
            if ((int)a1->s4.sin_addr.s_addr == (int)a2->s4.sin_addr.s_addr)
            {
                return true;
            }
        }
        else if (a1->ss.sa_family == AF_INET6 && a1->s6.sin6_port == a2->s6.sin6_port)
        {
            if (memcmp(&(a1->s6.sin6_addr), &(a2->s6.sin6_addr), sizeof(struct in6_addr)) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

bool addr_eq_no_port(const ioa_addr *a1, const ioa_addr *a2)
{
    if (!a1)
    {
        return (!a2);
    }
    else if (!a2)
    {
        return (!a1);
    }

    if (a1->ss.sa_family == a2->ss.sa_family)
    {
        if (a1->ss.sa_family == AF_INET)
        {
            if ((int)a1->s4.sin_addr.s_addr == (int)a2->s4.sin_addr.s_addr)
            {
                return true;
            }
        }
        else if (a1->ss.sa_family == AF_INET6)
        {
            if (memcmp(&(a1->s6.sin6_addr), &(a2->s6.sin6_addr), sizeof(struct in6_addr)) == 0)
            {
                return true;
            }
        }
    }
    return false;
}
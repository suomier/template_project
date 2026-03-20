#include "socket.h"

#include "base/common_def.h"
#include "network/network_def.h"

#include "net_utils.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief TTL 忽略标记值
 */
#define TTL_IGNORE ((int)(-1))

/**
 * @brief TTL 默认值
 */
#define TTL_DEFAULT (64)

/**
 * @brief TOS 忽略标记值
 */
#define TOS_IGNORE ((int)(-1))

/**
 * @brief TOS 默认值
 */
#define TOS_DEFAULT (0)

///////////////////////// ttl/tos //////////////////////////

#ifdef CORRECT_RAW_TTL
#undef CORRECT_RAW_TTL
#endif

/**
 * @brief 校正 TTL 值到合法范围（0-255），超出范围则使用默认值
 */
#define CORRECT_RAW_TTL(ttl)      \
    do                            \
    {                             \
        if (ttl < 0 || ttl > 255) \
            ttl = TTL_DEFAULT;    \
    } while (0)

#ifdef CORRECT_RAW_TOS
#undef CORRECT_RAW_TOS
#endif

/**
 * @brief 校正 TOS 值到合法范围（0-255），超出范围则使用默认值
 */
#define CORRECT_RAW_TOS(tos)      \
    do                            \
    {                             \
        if (tos < 0 || tos > 255) \
            tos = TOS_DEFAULT;    \
    } while (0)

int socket_init(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        return -1;
    }
#endif
    return 0;
}

void socket_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

bool is_sctp_socket(SOCKET_TYPE st)
{
    return (st == SCTP_SOCKET || st == TLS_SCTP_SOCKET ||
            st == TENTATIVE_SCTP_SOCKET);
}

int socket_set_reusable(socket_t fd, int flag, SOCKET_TYPE st, int use_reuseaddr)
{
    UNUSED_ARG(st);

    /* 检查 socket 文件描述符是否有效 */
    if (fd < 0)
    {
        return -1;
    }
    else
    {

/* SO_REUSEADDR: 允许地址复用
 * 该选项允许多个 socket 绑定到相同的 IP 地址和端口。
 * 主要用途：
 * 1. 服务器重启后立即重启，无需等待 TIME_WAIT 状态结束
 * 2. 多个进程/线程监听同一端口
 *
 * 注意：在某些系统上，只有当所有 socket 都设置此选项时，才能实现真正的复用
 */
#if defined(SO_REUSEADDR)
        if (use_reuseaddr)
        {
            int on = flag; /* 1=启用，0=禁用 */
            int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, (socklen_t)sizeof(on));
            if (ret < 0)
            {
                perror("SO_REUSEADDR");
            }
        }
#endif

/* SCTP_REUSE_PORT: SCTP 协议的端口复用选项
 * SCTP（Stream Control Transmission Protocol）是一种传输层协议。
 * 该选项类似于 SO_REUSEPORT，但专门用于 SCTP socket。
 * 只有在未禁用 SCTP 且系统支持 SCTP_REUSE_PORT 时才会设置此选项。
 */
#if !defined(TURN_NO_SCTP) && defined(SCTP_REUSE_PORT)
        if (use_reuseaddr)
        {
            /* 检查是否为 SCTP socket */
            if (is_sctp_socket(st))
            {
                int on = flag; /* 1=启用，0=禁用 */
                int ret = setsockopt(fd, IPPROTO_SCTP, SCTP_REUSE_PORT, (const char *)&on, (socklen_t)sizeof(on));
                if (ret < 0)
                {
                    perror("SCTP_REUSE_PORT");
                }
            }
        }
#endif

/* SO_REUSEPORT: 允许端口复用（BSD 和 Linux 3.9+）
 * 该选项允许多个 socket（通常在不同进程/线程中）绑定到完全相同的地址和端口。
 * 与 SO_REUSEADDR 不同，SO_REUSEPORT 会根据连接的负载均衡策略将连接分发到各个 socket。
 *
 * 注意：此选项并非所有平台都支持，需要特定的内核版本
 */
#if defined(SO_REUSEPORT)
        if (use_reuseaddr)
        {
            int on = flag; /* 1=启用，0=禁用 */
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&on, (socklen_t)sizeof(on));
        }
#endif

        return 0;
    }
}

int socket_set_nonblocking(socket_t fd)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(WINDOWS)
    /* Windows 平台：使用 ioctlsocket 设置非阻塞模式
     * FIONBIO: File I/O Non-Blocking I/O
     * nonblocking = 1 表示启用非阻塞模式
     */
    unsigned long nonblocking = 1;
    if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR)
    {
        perror("ioctlsocket FIONBIO");
        return -1;
    }
#else
    /* Unix/Linux 平台：使用 fcntl 设置文件状态标志
     * F_SETFL: 设置文件状态标志
     * O_NONBLOCK: 非阻塞标志
     * 先获取当前标志，然后添加 O_NONBLOCK 标志
     */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        return -1;
    }
#endif
    return 0;
}

int socket_tcp_set_keepalive(socket_t fd, SOCKET_TYPE st)
{
    UNUSED_ARG(st)

/* SO_KEEPALIVE: 启用 TCP keepalive 机制
 * Keepalive 是 TCP 协议的一个特性，用于检测连接是否仍然存活。
 * 工作原理：
 *   1. 在连接空闲一段时间后（由系统参数 net.ipv4.tcp_keepalive_time 控制）
 *   2. 系统发送一个 TCP keepalive 探测包
 *   3. 如果收到 ACK 响应，连接仍然有效，继续使用
 *   4. 如果多次探测无响应（由系统参数 net.ipv4.tcp_keepalive_probes 控制）
 *   5. 系统关闭连接并向应用程序返回错误
 *
 * Keepalive 的优势：
 *   - 自动检测死连接，释放资源
 *   - 防止网络中断导致的连接泄漏
 *   - 特别适用于长连接和需要维护连接状态的场景
 */
#ifdef SO_KEEPALIVE
    {
        int on = 1; /* 启用 keepalive */
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, (socklen_t)sizeof(on));
    }
#else
    UNUSED_ARG(fd);
#endif

/* SO_NOSIGPIPE: 禁用 SIGPIPE 信号（BSD/MacOS 系统）
 * 在 BSD 系列系统（包括 macOS）上，向已断开的 TCP 连接写入数据时，
 * 内核会向进程发送 SIGPIPE 信号。默认情况下，这个信号会终止进程。
 *
 * 设置 SO_NOSIGPIPE 选项后：
 *   - 向已断开连接写入不会触发 SIGPIPE 信号
 *   - write/send 函数会返回 -1，errno 设置为 EPIPE
 *   - 应用程序可以通过检查返回值来优雅地处理连接断开
 *
 * 为什么需要此选项：
 *   - 避免 SIGPIPE 信号意外终止进程
 *   - 让应用程序有更多控制权来处理连接错误
 *   - 提高程序的健壮性和稳定性
 *
 * 注意：Linux 不支持此选项，需要通过 MSG_NOSIGNAL 标志或忽略 SIGPIPE 信号来实现类似功能
 */
#ifdef SO_NOSIGPIPE
    {
        int on = 1; /* 启用 NOSIGPIPE */
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (const void *)&on, (socklen_t)sizeof(on));
    }
#endif

    return 0;
}

int set_raw_socket_tos(socket_t fd, int family, int tos)
{

    /* IPv6 地址族：使用 IPV6_TCLASS 选项
     * IPV6_TCLASS（Traffic Class）是 IPv6 协议中对应 IPv4 TOS 的字段
     * 它包含 8 位，用于指定 IPv6 数据包的流量类别
     */
    if (family == AF_INET6)
    {
#if !defined(IPV6_TCLASS)
        /* 平台不支持 IPV6_TCLASS 选项 */
        UNUSED_ARG(fd);
        UNUSED_ARG(tos);
#else
        /* CORRECT_RAW_TOS: 对 TOS 值进行必要的修正
         * 某些平台可能需要对 TOS 值进行特殊的位操作或格式转换
         */
        CORRECT_RAW_TOS(tos);
        /* 设置 IPv6 流量类别字段 */
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, (const char *)&tos, sizeof(tos)) < 0)
        {
            perror("set TCLASS on socket");
            return -1;
        }
#endif
    }
    else
    {
        /* IPv4 地址族：使用 IP_TOS 选项
         * IP_TOS（Type of Service）是 IPv4 协议头中的服务类型字段
         * 它包含 8 位，用于指定 IPv4 数据包的服务质量要求
         *
         * TOS 字段结构：
         *   - 前 3 位：优先级（Precedence）- 0-7
         *   - 后 4 位：服务类型（D/T/R/C）- 延迟/吞吐量/可靠性/成本
         *   - 最后 1 位：必须为 0
         *
         * 常见的 TOS 值：
         *   - 0x00: 普通服务
         *   - 0x10: 最小延迟（Minimize Delay）
         *   - 0x08: 最大吞吐量（Maximize Throughput）
         *   - 0x04: 最大可靠性（Maximize Reliability）
         *   - 0x02: 最小成本（Minimize Monetary Cost）
         */
#if !defined(IP_TOS)
        /* 平台不支持 IP_TOS 选项 */
        UNUSED_ARG(fd);
        UNUSED_ARG(tos);
#else
        /* 设置 IPv4 服务类型字段 */
        if (setsockopt(fd, IPPROTO_IP, IP_TOS, (const char *)&tos, sizeof(tos)) < 0)
        {
            perror("set TOS on socket");
            return -1;
        }
#endif
    }

    return 0;
}

int get_raw_socket_tos(socket_t fd, int family)
{
    int tos = 0;

    /* IPv6 地址族：使用 IPV6_TCLASS 选项
     * IPV6_TCLASS（Traffic Class）是 IPv6 协议中对应 IPv4 TOS 的字段
     * 它包含 8 位，用于指定 IPv6 数据包的流量类别
     */
    if (family == AF_INET6)
    {
#if !defined(IPV6_TCLASS)
        /* 平台不支持 IPV6_TCLASS 选项
         * 使用 do...while(0) 结构是为了方便扩展和代码一致性
         * 返回 TOS_IGNORE 表示平台不支持此功能
         */
        UNUSED_ARG(fd);
        do
        {
            return TOS_IGNORE;
        } while (0);
#else
        /* 使用 getsockopt 获取当前设置的 IPv6 流量类别值
         * slen 是选项值的长度（输入/输出参数）
         * 调用前 slen 设置为 sizeof(tos)
         * 调用后 slen 包含实际返回的值的大小
         */
        socklen_t slen = (socklen_t)sizeof(tos);
        if (getsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, (char *)&tos, &slen) < 0)
        {
            perror("get TCLASS on socket");
            return -1;
        }
#endif
    }
    else
    {
        /* IPv4 地址族：使用 IP_TOS 选项
         * IP_TOS（Type of Service）是 IPv4 协议头中的服务类型字段
         * 它包含 8 位，用于指定 IPv4 数据包的服务质量要求
         *
         * TOS 字段结构和常见值：
         *   - 0x00: 普通服务（Normal Service）
         *   - 0x10: 最小延迟（Minimize Delay）- 用于交互式应用
         *   - 0x08: 最大吞吐量（Maximize Throughput）- 用于大文件传输
         *   - 0x04: 最大可靠性（Maximize Reliability）- 用于关键数据
         *   - 0x02: 最小成本（Minimize Monetary Cost）- 用于低成本路由
         */
#if !defined(IP_TOS)
        /* 平台不支持 IP_TOS 选项
         * 返回 TOS_IGNORE 表示平台不支持此功能
         */
        UNUSED_ARG(fd);
        do
        {
            return TOS_IGNORE;
        } while (0);
#else
        /* 使用 getsockopt 获取当前设置的 IPv4 服务类型值
         * slen 是选项值的长度
         */
        socklen_t slen = (socklen_t)sizeof(tos);
        if (getsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&tos, &slen) < 0)
        {
            perror("get TOS on socket");
            return -1;
        }
#endif
    }

    /* CORRECT_RAW_TOS: 对 TOS 值进行必要的修正
     * 某些平台可能需要对从内核获取的 TOS 值进行特殊处理
     * 例如：位掩码操作、字节序转换等
     * 这确保了跨平台的兼容性
     */
    CORRECT_RAW_TOS(tos);

    return tos;
}

int set_raw_socket_ttl(socket_t fd, int family, int ttl)
{

    /* IPv6 地址族：使用 IPV6_UNICAST_HOPS 选项
     * 在 IPv6 中，TTL 的概念被重命名为 Hop Limit（跳数限制）
     * IPV6_UNICAST_HOPS 用于设置单播数据包的跳数限制
     * 虽然名称不同，但功能与 IPv4 的 TTL 完全相同
     */
    if (family == AF_INET6)
    {
#if !defined(IPV6_UNICAST_HOPS)
        /* 平台不支持 IPV6_UNICAST_HOPS 选项 */
        UNUSED_ARG(fd);
        UNUSED_ARG(ttl);
#else
        /* CORRECT_RAW_TTL: 对 TTL 值进行必要的修正
         * 某些平台可能需要对 TTL 值进行特殊的位操作或格式转换
         */
        CORRECT_RAW_TTL(ttl);
        /* 设置 IPv6 单播数据包的跳数限制 */
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (const char *)&ttl, sizeof(ttl)) < 0)
        {
            perror("set HOPLIMIT on socket");
            return -1;
        }
#endif
    }
    else
    {
        /* IPv4 地址族：使用 IP_TTL 选项
         * IP_TTL（Time To Live）是 IPv4 协议头中的生存时间字段
         * 它是一个 8 位无符号整数，范围是 1-255
         *
         * TTL 的工作原理：
         *   1. 发送端设置初始 TTL 值
         *   2. 每个路由器在转发数据包时将 TTL 减 1
         *   3. 当 TTL 减到 0 时，路由器丢弃数据包
         *   4. 路由器向发送端发送 ICMP Time Exceeded 消息
         *
         * TTL 的实际应用：
         *   - 防止路由环路：确保数据包不会在路由器之间无限循环
         *   - Traceroute 工具：通过逐步增加 TTL 值来追踪路由路径
         *   - 网络诊断：帮助定位网络故障点
         *   - 性能调优：在某些情况下调整 TTL 可以改善网络性能
         */
#if !defined(IP_TTL)
        /* 平台不支持 IP_TTL 选项 */
        UNUSED_ARG(fd);
        UNUSED_ARG(ttl);
#else
        /* CORRECT_RAW_TTL: 对 TTL 值进行必要的修正
         * 某些平台可能需要对 TTL 值进行特殊处理
         */
        CORRECT_RAW_TTL(ttl);
        /* 设置 IPv4 数据包的生存时间 */
        if (setsockopt(fd, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl)) < 0)
        {
            perror("set TTL on socket");
            return -1;
        }
#endif
    }

    return 0;
}

int get_raw_socket_ttl(socket_t fd, int family)
{
    int ttl = 0;

    /* IPv6 地址族：使用 IPV6_UNICAST_HOPS 选项
     * 在 IPv6 中，TTL 的概念被重命名为 Hop Limit（跳数限制）
     * IPV6_UNICAST_HOPS 用于获取单播数据包的跳数限制
     * 虽然名称不同，但功能与 IPv4 的 TTL 完全相同
     */
    if (family == AF_INET6)
    {
#if !defined(IPV6_UNICAST_HOPS)
        /* 平台不支持 IPV6_UNICAST_HOPS 选项
         * 使用 do...while(0) 结构是为了方便扩展和代码一致性
         * 返回 TTL_IGNORE 表示平台不支持此功能
         */
        UNUSED_ARG(fd);
        do
        {
            return TTL_IGNORE;
        } while (0);
#else
        /* 使用 getsockopt 获取当前设置的 IPv6 跳数限制值
         * slen 是选项值的长度（输入/输出参数）
         * 调用前 slen 设置为 sizeof(ttl)
         * 调用后 slen 包含实际返回的值的大小
         */
        socklen_t slen = (socklen_t)sizeof(ttl);
        if (getsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (char *)&ttl, &slen) < 0)
        {
            perror("get HOPLIMIT on socket");
            return TTL_IGNORE;
        }
#endif
    }
    else
    {
        /* IPv4 地址族：使用 IP_TTL 选项
         * IP_TTL（Time To Live）是 IPv4 协议头中的生存时间字段
         * 它是一个 8 位无符号整数，范围是 1-255
         *
         * TTL 的工作机制：
         *   1. 发送端设置初始 TTL 值
         *   2. 每个路由器在转发数据包时将 TTL 减 1
         *   3. 当 TTL 减到 0 时，路由器丢弃数据包
         *   4. 路由器向发送端发送 ICMP Time Exceeded 消息
         *   5. 发送端收到 ICMP 消息，可以判断路由路径或故障点
         *
         * Traceroute 原理：
         *   Traceroute 工具利用 TTL 机制来发现网络路径：
         *     - 第一个包 TTL=1，在第一个路由器超时，返回 ICMP 消息
         *     - 第二个包 TTL=2，在第二个路由器超时，返回 ICMP 消息
         *     - 以此类推，直到到达目标主机
         *   这样就可以逐步发现完整的路由路径
         */
#if !defined(IP_TTL)
        /* 平台不支持 IP_TTL 选项
         * 返回 TTL_IGNORE 表示平台不支持此功能
         */
        UNUSED_ARG(fd);
        do
        {
            return TTL_IGNORE;
        } while (0);
#else
        /* 使用 getsockopt 获取当前设置的 IPv4 TTL 值
         * slen 是选项值的长度
         */
        socklen_t slen = (socklen_t)sizeof(ttl);
        if (getsockopt(fd, IPPROTO_IP, IP_TTL, (char *)&ttl, &slen) < 0)
        {
            perror("get TTL on socket");
            return TTL_IGNORE;
        }
#endif
    }

    /* CORRECT_RAW_TTL: 对 TTL 值进行必要的修正
     * 某些平台可能需要对从内核获取的 TTL 值进行特殊处理
     * 例如：位掩码操作、字节序转换、范围验证等
     * 这确保了跨平台的兼容性
     */
    CORRECT_RAW_TTL(ttl);

    return ttl;
}

int set_sock_buf_size(socket_t fd, int sz0)
{
    int sz;

    /* ========== 设置接收缓冲区 (SO_RCVBUF) ========== */
    /* SO_RCVBUF: Socket Receive Buffer Size
     * 接收缓冲区用于存放从网络接收但尚未被应用程序读取的数据。
     *
     * 工作原理：
     *   - 当数据从网络到达时，首先存放在接收缓冲区
     *   - 应用程序调用 recv/read 从缓冲区读取数据
     *   - 如果缓冲区已满，新到达的数据会被丢弃
     *   - 如果缓冲区太小，可能导致数据丢失和性能下降
     *
     * 系统限制：
     *   - Linux: /proc/sys/net/core/rmem_max 定义了最大接收缓冲区
     *   - 实际设置的值通常是请求值的两倍（系统额外保留空间）
     */
    sz = sz0;
    while (sz > 0)
    {
        /* 尝试设置接收缓冲区大小
         * 如果设置失败（通常是因为超出系统限制），将大小减半重试
         * 这种逐步减半的策略可以找到系统接受的最大可用大小
         */
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&sz, (socklen_t)sizeof(sz)) < 0)
        {
            sz = sz / 2;
        }
        else
        {
            break;
        }
    }

    /* 检查最终设置的缓冲区大小是否有效
     * 如果 sz < 1，说明系统无法设置任何接收缓冲区大小
     * 这通常表示系统资源不足或权限问题
     */
    if (sz < 1)
    {
        perror("Cannot set socket rcv size");
    }

    /* ========== 设置发送缓冲区 (SO_SNDBUF) ========== */
    /* SO_SNDBUF: Socket Send Buffer Size
     * 发送缓冲区用于存放应用程序已发送但尚未被网络传输的数据。
     *
     * 工作原理：
     *   - 应用程序调用 send/write 时，数据首先放入发送缓冲区
     *   - 系统的 TCP/IP 栈从缓冲区读取数据并传输到网络
     *   - 如果缓冲区已满，send/write 会阻塞或返回 EAGAIN（非阻塞模式）
     *   - 较大的发送缓冲区可以提高发送吞吐量
     *
     * 系统限制：
     *   - Linux: /proc/sys/net/core/wmem_max 定义了最大发送缓冲区
     *   - 实际设置的值通常是请求值的两倍（系统额外保留空间）
     */
    sz = sz0;
    while (sz > 0)
    {
        /* 尝试设置发送缓冲区大小
         * 使用与接收缓冲区相同的逐步减半策略
         */
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&sz, (socklen_t)sizeof(sz)) < 0)
        {
            sz = sz / 2;
        }
        else
        {
            break;
        }
    }

    /* 检查最终设置的缓冲区大小是否有效
     * 如果 sz < 1，说明系统无法设置任何发送缓冲区大小
     */
    if (sz < 1)
    {
        perror("Cannot set socket snd size");
    }

    return 0;
}

int addr_connect(socket_t fd, const ioa_addr *addr, int *out_errno)
{
    /* ========== 参数有效性检查 ========== */
    /* 检查地址指针和 socket 文件描述符是否有效
     * addr 不能为 NULL，否则无法知道要连接的目标地址
     * fd 必须 >= 0，负数表示无效的文件描述符
     */
    if (!addr || fd < 0)
    {
        return -1;
    }
    else
    {
        int err = 0;
        /* ========== 连接循环（处理中断） ========== */
        /* 使用 do-while 循环来执行连接操作
         * 循环条件：连接失败 && 错误是 EINTR（被信号中断）
         *
         * 为什么要处理 EINTR：
         *   - connect() 可能被信号中断（如处理其他 I/O）
         *   - 这不是真正的错误，应该重试
         *   - socket_eintr() 检查当前错误是否为 EINTR
         *   - 通过循环重试，可以确保连接操作完成
         */
        do
        {
            /* 根据地址族调用对应的 connect()
             * IPv4 和 IPv6 使用不同的地址结构大小
             * ioa_addr 是一个通用地址结构，可以容纳任何地址族
             */
            if (addr->ss.sa_family == AF_INET)
            {
                /* IPv4 地址族
                 * 使用 sockaddr_in 结构，大小为 sizeof(struct sockaddr_in)
                 * AF_INET: Address Family Internet（IPv4）
                 */
                err = connect(fd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in));
            }
            else if (addr->ss.sa_family == AF_INET6)
            {
                /* IPv6 地址族
                 * 使用 sockaddr_in6 结构，大小为 sizeof(struct sockaddr_in6)
                 * AF_INET6: Address Family Internet version 6
                 */
                err = connect(fd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in6));
            }
            else
            {
                /* 不支持的地址族
                 * 既不是 IPv4 也不是 IPv6，无法处理
                 * 直接返回错误
                 */
                return -1;
            }
        } while (err < 0 && socket_eintr());

        /* ========== 保存错误码 ========== */
        /* 如果调用者提供了 out_errno 参数，保存当前的系统错误码
         * 这允许调用者获取详细的错误信息，而不仅仅是知道失败
         * 常见错误码：
         *   - ECONNREFUSED: 连接被拒绝（端口未监听或被防火墙阻止）
         *   - ETIMEDOUT: 连接超时
         *   - EHOSTUNREACH: 主机不可达
         *   - ENETUNREACH: 网络不可达
         *   - EINPROGRESS: 非阻塞 socket 连接正在异步进行
         */
        if (out_errno)
        {
            *out_errno = socket_errno();
        }

        /* ========== 错误处理和日志 ========== */
        /* 检查连接是否失败，且错误不是 EINPROGRESS
         * socket_einprogress(): 检查当前错误是否为 EINPROGRESS
         *
         * 为什么不记录 EINPROGRESS：
         *   - EINPROGRESS 不是错误，而是非阻塞连接的正常状态
         *   - 表示连接操作正在后台进行中
         *   - 应用程序应该等待连接完成，而不是视为错误
         *
         * 何时打印错误：
         *   - 只有真正的连接失败（如被拒绝、超时、网络不可达）才打印错误
         *   - perror() 会将错误码转换为可读的错误描述
         */
        if (err < 0 && !socket_einprogress())
        {
            perror("Connect");
        }

        return err;
    }
}

int sock_bind_to_device(socket_t fd, const unsigned char *ifname)
{
    /* ========== 参数有效性检查 ========== */
    /* 检查 socket 文件描述符和接口名称是否有效
     * fd >= 0: 确保是有效的文件描述符
     * ifname && ifname[0]: 确保接口名称指针非空且非空字符串
     */
    if (fd >= 0 && ifname && ifname[0])
    {
#if defined(SO_BINDTODEVICE)
        /* ========== 设置 SO_BINDTODEVICE 选项 ========== */
        /* SO_BINDTODEVICE: 将 socket 绑定到特定的网络设备
         *
         * 工作原理：
         *   1. 应用程序调用 setsockopt 设置此选项
         *   2. 内核将 socket 与指定的网络接口关联
         *   3. 所有通过该 socket 的流量都通过该接口
         *   4. 如果接口不存在或无效，绑定会失败
         *
         * 限制和注意事项：
         *   - 需要 CAP_NET_RAW 或 root 权限（普通用户无法使用）
         *   - 不是所有平台都支持此选项（Windows 不支持）
         *   - 绑定后，socket 不能更改绑定的设备
         *   - 对 TCP socket，连接后绑定不会生效
         *
         * ifreq 结构：
         *   - struct ifreq 用于接口配置和查询
         *   - ifr_name 字段：接口名称（如 "eth0"）
         *   - 其他字段：用于不同的 ioctl 操作
         */
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));

        /* 复制接口名称到 ifreq 结构
         * strncpy 不会自动添加 null 终止符，需要手动添加
         * ifr.ifr_name 的大小通常是 IFNAMSIZ（16 字节）
         */
        strncpy(ifr.ifr_name, (const char *)ifname, sizeof(ifr.ifr_name));

        /* 调用 setsockopt 绑定到指定设备
         * 参数说明：
         *   - fd: socket 文件描述符
         *   - SOL_SOCKET: socket 级别选项
         *   - SO_BINDTODEVICE: 绑定到设备选项
         *   - &ifr: 接口名称结构体
         *   - sizeof(ifr): 结构体大小
         */
        if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&ifr, sizeof(ifr)) < 0)
        {
            /* ========== 错误处理 ========== */
            /* 检查错误是否为权限不足
             * EPERM: Permission Denied
             *   - 用户没有足够的权限绑定到网络设备
             *   - 需要 root 权限或 CAP_NET_RAW capability
             *   - 在 Linux 上，普通用户无法使用此功能
             */
            if (socket_eperm())
            {
                perror("You must obtain superuser privileges to bind a socket to device");
            }
            else
            {
                /* 其他错误：可能是设备不存在或系统不支持
                 * 常见错误：
                 *   - ENODEV: 设备不存在
                 *   - ENOTCONN: socket 已连接（TCP）
                 *   - EINVAL: 参数无效
                 *   - EOPNOTSUPP: 系统不支持此选项
                 */
                perror("Cannot bind socket to device");
            }

            return -1;
        }

        /* 绑定成功 */
        return 0;

#endif
    }

    /* 参数无效或平台不支持 SO_BINDTODEVICE
     * 如果平台不支持，静默返回成功（忽略请求）
     */
    return 0;
}

int addr_bind(socket_t fd, const ioa_addr *addr, int reusable, int debug, SOCKET_TYPE st)
{
    /* ========== 参数有效性检查 ========== */
    /* 检查地址指针和 socket 文件描述符是否有效
     * addr 不能为 NULL，否则无法知道要绑定的目标地址
     * fd 必须 >= 0，负数表示无效的文件描述符
     */
    if (!addr || fd < 0)
    {
        return -1;
    }
    else
    {
        int ret = -1;
        /* ========== 设置地址复用选项 ========== */
        /* 调用 socket_set_reusable() 设置 SO_REUSEADDR 选项
         * 允许多个 socket 绑定到相同的地址和端口
         * 这对于服务器重启和负载均衡非常重要
         */
        socket_set_reusable(fd, reusable, st, 1);

        /* ========== 根据地址族执行绑定 ========== */
        if (addr->ss.sa_family == AF_INET)
        {
            /* IPv4 地址族处理
             * AF_INET: Address Family Internet（IPv4）
             * 使用 sockaddr_in 结构，大小为 sizeof(struct sockaddr_in)
             */
            do
            {
                /* 调用 bind() 绑定到 IPv4 地址
                 * 循环处理 EINTR 错误（被信号中断）
                 */
                ret = bind(fd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in));
            } while (ret < 0 && socket_eintr());
        }
        else if (addr->ss.sa_family == AF_INET6)
        {
            /* IPv6 地址族处理
             * AF_INET6: Address Family Internet version 6
             * 使用 sockaddr_in6 结构，大小为 sizeof(struct sockaddr_in6)
             */

            /* ========== 设置 IPV6_V6ONLY 选项 ========== */
            /* IPV6_V6ONLY: 控制 IPv6 socket 是否接收 IPv4 数据包
             *
             * 设置 off=0（禁用 IPV6_V6ONLY）：
             *   - IPv6 socket 可以同时接收 IPv4 和 IPv6 流量
             *   - 这称为"双栈"或"映射地址"模式
             *   - IPv4 地址会映射为 IPv6 格式（::ffff:x.x.x.x）
             *   - 适用于需要同时支持两种协议的应用
             *
             * 设置 off=1（启用 IPV6_V6ONLY）：
             *   - IPv6 socket 只接收 IPv6 流量
             *   - 更清晰和可预测的行为
             *   - 避免地址混淆和安全问题
             *
             * 为什么设置为 0：
             *   - 允许 TURN 服务器同时支持 IPv4 和 IPv6 客户端
             *   - 减少需要创建的 socket 数量
             *   - 简化服务器配置
             */
            const int off = 0;
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&off, sizeof(off));

            /* 调用 bind() 绑定到 IPv6 地址
             * 循环处理 EINTR 错误（被信号中断）
             */
            do
            {
                ret = bind(fd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in6));
            } while (ret < 0 && socket_eintr());
        }
        else
        {
            /* 不支持的地址族
             * 既不是 IPv4 也不是 IPv6，无法处理
             * 直接返回错误
             */
            return -1;
        }

        /* ========== 绑定结果处理 ========== */
        /* 检查绑定是否失败
         * ret < 0 表示 bind() 调用失败
         * 常见失败原因：
         *   - EADDRINUSE: 地址已被使用
         *   - EADDRNOTAVAIL: 地址不可用
         *   - EACCES: 权限不足（绑定特权端口 < 1024）
         *   - EAFNOSUPPORT: 不支持的地址族
         */
        if (ret < 0)
        {
            /* 如果启用了调试模式，输出详细的错误信息
             * debug 参数允许在开发和调试时获得更多信息
             */
            if (debug)
            {
                /* 获取系统错误码 */
                int err = socket_errno();
                /* 打印 bind 错误描述 */
                perror("bind");
                /* 将地址转换为可读的字符串格式
                 * str 数组用于存储地址字符串（如 "192.168.1.1:3478"）
                 */
                char str[129];
                addr_to_string(addr, (uint8_t *)str);
            }
        }
        return ret;
    }
}

int addr_get_from_sock(socket_t fd, ioa_addr *addr)
{
    if (fd < 0 || !addr)
    {
        return -1;
    }
    else
    {
        ioa_addr a;
        a.ss.sa_family = AF_INET6;
        socklen_t socklen = get_ioa_addr_len(&a);
        if (getsockname(fd, (struct sockaddr *)&a, &socklen) < 0)
        {
            a.ss.sa_family = AF_INET;
            socklen = get_ioa_addr_len(&a);
            if (getsockname(fd, (struct sockaddr *)&a, &socklen) < 0)
            {
                return -1;
            }
        }

        addr_cpy(addr, &a);
        return 0;
    }
}

uint32_t get_ioa_addr_len(const ioa_addr *addr)
{
    if (addr->ss.sa_family == AF_INET)
    {
        return sizeof(struct sockaddr_in);
    }
    else if (addr->ss.sa_family == AF_INET6)
    {
        return sizeof(struct sockaddr_in6);
    }
    return 0;
}

void addr_set_port(ioa_addr *addr, int port)
{
    if (addr)
    {
        if (addr->s4.sin_family == AF_INET)
        {
            addr->s4.sin_port = nswap16(port);
        }
        else if (addr->s6.sin6_family == AF_INET6)
        {
            addr->s6.sin6_port = nswap16(port);
        }
    }
}

int addr_get_port(const ioa_addr *addr)
{
    if (!addr)
    {
        return 0;
    }

    if (addr->s4.sin_family == AF_INET)
    {
        return nswap16(addr->s4.sin_port);
    }
    else if (addr->s6.sin6_family == AF_INET6)
    {
        return nswap16(addr->s6.sin6_port);
    }
    return 0;
}

void read_spare_buffer(socket_t fd)
{
    if (fd >= 0)
    {
        char buffer[65536];
#if defined(WINDOWS)
        // Windows平台：使用非阻塞模式读取并丢弃数据
        // 首先检查是否为非阻塞模式，如果不是则忽略
        unsigned long mode = 0;
        ioctlsocket(fd, FIONBIO, &mode);
        recv(fd, buffer, sizeof(buffer), 0);
#else
        // Unix/Linux平台：使用MSG_DONTWAIT标志实现非阻塞读取
        recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
#endif
    }
}

bool handle_socket_error(void)
{
    /* EINTR: 系统调用被信号中断
     * 这是一种正常情况，通常发生在应用程序处理信号时。
     * 操作可以安全重试，不影响连接的正常使用。
     */
    if (socket_eintr())
    {
        return true;
    }

    /* ENOBUFS: 系统缓冲区空间不足
     * 通常发生在网络拥塞时，系统没有足够的缓冲区空间来接收数据。
     * 这是一种暂时性的网络状况，可以等待一段时间后重试。
     */
    if (socket_enobufs())
    {
        return true;
    }

    /* EWOULDBLOCK 或 EAGAIN: 非阻塞操作需要等待
     * 在非阻塞 socket 上调用 I/O 函数时，如果没有数据可读或无法立即写入，
     * 会返回这个错误。这是非阻塞 I/O 的正常行为，不是错误情况。
     * 应用程序应该稍后再次尝试该操作（通常使用 select/poll/epoll 等机制）。
     */
    if (socket_ewouldblock() || socket_eagain())
    {
        return true;
    }

    /* EBADF: 无效的文件描述符
     * socket 已经被关闭或未正确创建，这是一个致命错误。
     * 必须关闭连接，不能继续使用这个 socket。
     */
    if (socket_ebadf())
    {
        return false;
    }

    /* EHOSTDOWN: 主机宕机
     * 通常表示目标主机不可达。可能是由网络故障引起，
     * 也可能是攻击者发送虚假的 ICMP 消息造成的伪造错误。
     * 为了安全起见，忽略此错误并继续，让上层逻辑决定是否重试。
     */
    if (socket_ehostdown())
    {
        return true;
    }

    /* ECONNRESET 或 ECONNREFUSED: 连接被重置或拒绝
     * ECONNRESET: 对端强制关闭了连接（通常是对端进程崩溃或网络重置）
     * ECONNREFUSED: 连接请求被拒绝（通常是对端没有监听该端口）
     * 这都是致命的连接错误，必须关闭连接并清理资源。
     */
    if (socket_econnreset() || socket_econnrefused())
    {
        return false;
    }

    /* ENOMEM: 内存不足
     * 系统内存耗尽，无法完成操作。这是严重错误，
     * 必须关闭连接以释放资源并尝试恢复系统状态。
     */
    if (socket_enomem())
    {
        return false;
    }

    /* EACCES: 权限被拒绝
     * 通常发生在尝试访问受限制的资源或被防火墙策略阻止时。
     * 这可能是暂时性的（如防火墙规则变更），可以重试并希望能成功。
     */
    if (socket_eacces())
    {
        return true;
    }

    /* 发生意外的错误
     * 此处处理未被上述逻辑捕获的错误，可能是未预料到的情况。
     * 出于安全考虑，将此类错误视为致命错误，关闭连接。
     */
    // TURN_LOG_FUNC(TURN_LOG_LEVEL_INFO, "Unexpected error! (errno = %d)\n", socket_errno());
    return false;
}

///////////////////////// socket creation/closing //////////////////////////

socket_t socket_create(int domain, int type, int protocol)
{
#ifdef _WIN32
    socket_t fd = (socket_t)::socket(domain, type, protocol);
    return (fd == INVALID_SOCKET) ? -1 : fd;
#else
    return ::socket(domain, type, protocol);
#endif
}

int socket_close(socket_t fd)
{
    if (fd < 0)
    {
        return -1;
    }

#ifdef _WIN32
    return closesocket(fd);
#else
    return close(fd);
#endif
}

///////////////////////// socket listen/accept //////////////////////////

int socket_listen(socket_t fd, int backlog)
{
    if (fd < 0)
    {
        return -1;
    }

    int ret = listen(fd, backlog);
    if (ret < 0)
    {
        perror("listen");
    }
    return ret;
}

socket_t socket_accept(socket_t fd, ioa_addr *client_addr)
{
    if (fd < 0)
    {
        return -1;
    }

    ioa_addr addr;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

#ifdef _WIN32
    socket_t client_fd = (socket_t)::accept(fd, (struct sockaddr *)&addr, &addr_len);
    if (client_fd == INVALID_SOCKET)
    {
        return -1;
    }
#else
    socket_t client_fd = ::accept(fd, (struct sockaddr *)&addr, &addr_len);
    if (client_fd < 0)
    {
        return -1;
    }
#endif

    if (client_addr)
    {
        addr_cpy(client_addr, &addr);
    }

    return client_fd;
}

///////////////////////// socket read/write //////////////////////////

int socket_send(socket_t fd, const void *buf, size_t len, int flags)
{
    if (fd < 0 || !buf)
    {
        return -1;
    }

#ifdef _WIN32
    return send(fd, (const char *)buf, (int)len, flags);
#else
    return (int)send(fd, buf, len, flags);
#endif
}

int socket_recv(socket_t fd, void *buf, size_t len, int flags)
{
    if (fd < 0 || !buf)
    {
        return -1;
    }

#ifdef _WIN32
    return recv(fd, (char *)buf, (int)len, flags);
#else
    return (int)recv(fd, buf, len, flags);
#endif
}

int socket_sendto(socket_t fd, const void *buf, size_t len, int flags, const ioa_addr *addr)
{
    if (fd < 0 || !buf || !addr)
    {
        return -1;
    }

    socklen_t addr_len = get_ioa_addr_len(addr);

#ifdef _WIN32
    return sendto(fd, (const char *)buf, (int)len, flags,
                  (const struct sockaddr *)addr, (int)addr_len);
#else
    return (int)sendto(fd, buf, len, flags, (const struct sockaddr *)addr, addr_len);
#endif
}

int socket_recvfrom(socket_t fd, void *buf, size_t len, int flags, ioa_addr *addr)
{
    if (fd < 0 || !buf)
    {
        return -1;
    }

    ioa_addr tmp_addr;
    socklen_t addr_len = sizeof(tmp_addr);
    memset(&tmp_addr, 0, sizeof(tmp_addr));

#ifdef _WIN32
    int ret = recvfrom(fd, (char *)buf, (int)len, flags,
                       (struct sockaddr *)&tmp_addr, &addr_len);
#else
    int ret = (int)recvfrom(fd, buf, len, flags, (struct sockaddr *)&tmp_addr, &addr_len);
#endif

    if (ret >= 0 && addr)
    {
        addr_cpy(addr, &tmp_addr);
    }

    return ret;
}

///////////////////////// socket options //////////////////////////

int socket_set_send_timeout(socket_t fd, int timeout_ms)
{
    if (fd < 0 || timeout_ms < 0)
    {
        return -1;
    }

#if defined(SO_SNDTIMEO)
#ifdef _WIN32
    DWORD timeout = timeout_ms;
    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
#endif
    if (ret < 0)
    {
        perror("setsockopt SO_SNDTIMEO");
    }
    return ret;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(timeout_ms);
    return -1;
#endif
}

int socket_set_recv_timeout(socket_t fd, int timeout_ms)
{
    if (fd < 0 || timeout_ms < 0)
    {
        return -1;
    }

#if defined(SO_RCVTIMEO)
#ifdef _WIN32
    DWORD timeout = timeout_ms;
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#endif
    if (ret < 0)
    {
        perror("setsockopt SO_RCVTIMEO");
    }
    return ret;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(timeout_ms);
    return -1;
#endif
}

int socket_get_send_timeout(socket_t fd, int *timeout_ms)
{
    if (fd < 0 || !timeout_ms)
    {
        return -1;
    }

#if defined(SO_SNDTIMEO)
#ifdef _WIN32
    DWORD timeout;
    socklen_t len = sizeof(timeout);
#else
    struct timeval tv;
    socklen_t len = sizeof(tv);
#endif

    if (getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, &len) < 0)
    {
        perror("getsockopt SO_SNDTIMEO");
        return -1;
    }

#ifdef _WIN32
    *timeout_ms = (int)timeout;
#else
    *timeout_ms = (int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    return 0;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(timeout_ms);
    return -1;
#endif
}

int socket_get_recv_timeout(socket_t fd, int *timeout_ms)
{
    if (fd < 0 || !timeout_ms)
    {
        return -1;
    }

#if defined(SO_RCVTIMEO)
#ifdef _WIN32
    DWORD timeout;
    socklen_t len = sizeof(timeout);
#else
    struct timeval tv;
    socklen_t len = sizeof(tv);
#endif

    if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, &len) < 0)
    {
        perror("getsockopt SO_RCVTIMEO");
        return -1;
    }

#ifdef _WIN32
    *timeout_ms = (int)timeout;
#else
    *timeout_ms = (int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    return 0;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(timeout_ms);
    return -1;
#endif
}

int socket_set_broadcast(socket_t fd, int enable)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(SO_BROADCAST)
    int on = enable ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on));
    if (ret < 0)
    {
        perror("setsockopt SO_BROADCAST");
    }
    return ret;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(enable);
    return -1;
#endif
}

int socket_get_broadcast(socket_t fd, int *enabled)
{
    if (fd < 0 || !enabled)
    {
        return -1;
    }

#if defined(SO_BROADCAST)
    int on = 0;
    socklen_t len = sizeof(on);
    if (getsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&on, &len) < 0)
    {
        perror("getsockopt SO_BROADCAST");
        return -1;
    }
    *enabled = on;
    return 0;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(enabled);
    return -1;
#endif
}

int socket_tcp_set_nodelay(socket_t fd, int enable)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(TCP_NODELAY)
    int on = enable ? 1 : 0;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
    if (ret < 0)
    {
        perror("setsockopt TCP_NODELAY");
    }
    return ret;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(enable);
    return -1;
#endif
}

int socket_tcp_get_nodelay(socket_t fd, int *enabled)
{
    if (fd < 0 || !enabled)
    {
        return -1;
    }

#if defined(TCP_NODELAY)
    int on = 0;
    socklen_t len = sizeof(on);
    if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, &len) < 0)
    {
        perror("getsockopt TCP_NODELAY");
        return -1;
    }
    *enabled = on;
    return 0;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(enabled);
    return -1;
#endif
}

int socket_tcp_set_keepalive_params(socket_t fd, int idle, int interval, int count)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(SO_KEEPALIVE)
    /* 启用 keepalive */
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_KEEPALIVE");
        return -1;
    }

#if defined(TCP_KEEPIDLE)
    /* 设置空闲时间（秒）- Linux */
    if (idle > 0 && setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (const char *)&idle, sizeof(idle)) < 0)
    {
        perror("setsockopt TCP_KEEPIDLE");
        return -1;
    }
#elif defined(TCP_KEEPALIVE)
    /* 设置空闲时间（秒）- Windows/macOS */
    if (idle > 0 && setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, (const char *)&idle, sizeof(idle)) < 0)
    {
        perror("setsockopt TCP_KEEPALIVE");
        return -1;
    }
#else
    UNUSED_ARG(idle);
#endif

#if defined(TCP_KEEPINTVL)
    /* 设置重试间隔（秒） */
    if (interval > 0 && setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (const char *)&interval, sizeof(interval)) < 0)
    {
        perror("setsockopt TCP_KEEPINTVL");
        return -1;
    }
#else
    UNUSED_ARG(interval);
#endif

#if defined(TCP_KEEPCNT)
    /* 设置重试次数 */
    if (count > 0 && setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (const char *)&count, sizeof(count)) < 0)
    {
        perror("setsockopt TCP_KEEPCNT");
        return -1;
    }
#else
    UNUSED_ARG(count);
#endif

    return 0;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(idle);
    UNUSED_ARG(interval);
    UNUSED_ARG(count);
    return -1;
#endif
}

int socket_set_linger(socket_t fd, int onoff, int linger_seconds)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(SO_LINGER)
    struct linger l;
    l.l_onoff = onoff;
    l.l_linger = linger_seconds;
    int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&l, sizeof(l));
    if (ret < 0)
    {
        perror("setsockopt SO_LINGER");
    }
    return ret;
#else
    UNUSED_ARG(fd);
    UNUSED_ARG(onoff);
    UNUSED_ARG(linger_seconds);
    return -1;
#endif
}

#if defined(SO_REUSEPORT)
int socket_set_reuseport(socket_t fd, int enable)
{
    if (fd < 0)
    {
        return -1;
    }

    int on = enable ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&on, sizeof(on));
    if (ret < 0)
    {
        perror("setsockopt SO_REUSEPORT");
    }
    return ret;
}
#endif

int socket_errno(void)
{
#if defined(WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool socket_eperm(void)
{
#if defined(WIN32)
    return false; // Windows 无此错误码
#else
    return socket_errno() == EPERM;
#endif
}

bool socket_enomem(void)
{
#if defined(WIN32)
    return socket_errno() == WSA_NOT_ENOUGH_MEMORY;
#else
    return socket_errno() == ENOMEM;
#endif
}

bool socket_eintr(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEINTR;
#else
    return socket_errno() == EINTR;
#endif
}

bool socket_ebadf(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEBADF;
#else
    return socket_errno() == EBADF;
#endif
}

bool socket_eacces(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEACCES;
#else
    return socket_errno() == EACCES;
#endif
}

bool socket_enobufs(void)
{
#if defined(WIN32)
    return socket_errno() == WSAENOBUFS;
#else
    return socket_errno() == ENOBUFS;
#endif
}

bool socket_eagain(void)
{
#if defined(WIN32)
    return socket_errno() == WSATRY_AGAIN;
#else
    return socket_errno() == EAGAIN;
#endif
}

bool socket_ewouldblock(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEWOULDBLOCK;
#else
#if defined(EWOULDBLOCK)
    return socket_errno() == EWOULDBLOCK;
#else
    return socket_errno() == EAGAIN;
#endif
#endif
}

bool socket_einprogress(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEINPROGRESS;
#else
    return socket_errno() == EINPROGRESS;
#endif
}

bool socket_econnreset(void)
{
#if defined(WIN32)
    return socket_errno() == WSAECONNRESET;
#else
    return socket_errno() == ECONNRESET;
#endif
}

bool socket_econnrefused(void)
{
#if defined(WIN32)
    return socket_errno() == WSAECONNREFUSED;
#else
    return socket_errno() == ECONNREFUSED;
#endif
}

bool socket_ehostdown(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEHOSTDOWN;
#else
    return socket_errno() == EHOSTDOWN;
#endif
}

bool socket_emsgsize(void)
{
#if defined(WIN32)
    return socket_errno() == WSAEMSGSIZE;
#else
    return socket_errno() == EMSGSIZE;
#endif
}
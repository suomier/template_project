#ifndef SOCKET_H
#define SOCKET_H

#include "network/network_def.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////// init/cleanup //////////////////////////

/**
 * @brief 初始化 Socket 库（仅 Windows 需要）
 * @details Windows 需要 WSAStartup，Unix/Linux 不需要
 * @return 成功返回 0，失败返回 -1
 */
int socket_init(void);

/**
 * @brief 清理 Socket 库（仅 Windows 需要）
 * @details Windows 需要 WSACleanup，Unix/Linux 不需要
 */
void socket_cleanup(void);

///////////////////////// socket attribute //////////////////////////

/**
 * @brief 判断是否为SCTP套接字
 * @param st 套接字类型
 * @return 是SCTP套接字返回true，否则返回false
 */
bool is_sctp_socket(SOCKET_TYPE st);

/**
 * @brief 设置套接字可重用（如 SO_REUSEADDR）
 *
 * @param fd 套接字文件描述符
 * @param reusable 是否可重用（0=不可重用，1=可重用）
 * @param st 套接字类型（TCP/UDP/TCP6/UDP6 等）
 * @param use_reuseaddr 是否启用地址复用; 1启用, 0不启用
 * @return 成功返回 0，失败返回 -1
 *
 * 允许套接字绑定到处于 TIME_WAIT 状态的地址，避免"地址已在使用"错误
 */
int socket_set_reusable(socket_t fd, int reusable, SOCKET_TYPE st, int use_reuseaddr);

/**
 * @brief 设置套接字为非阻塞模式
 *
 * @param fd 套接字文件描述符
 * @return 成功返回 0，失败返回 -1
 *
 * 设置套接字为非阻塞 I/O 模式，使 socket 操作立即返回而不阻塞
 */
int socket_set_nonblocking(socket_t fd);

/**
 * @brief 设置 TCP 套接字的 keepalive 选项
 *
 * @param fd 套接字文件描述符
 * @param st 套接字类型
 * @return 成功返回 0，失败返回 -1
 *
 * 启用 TCP keepalive 机制，用于检测死连接并自动断开
 */
int socket_tcp_set_keepalive(socket_t fd, SOCKET_TYPE st);

/**
 * @brief 设置原始套接字的 TOS（服务类型）
 *
 * @param fd 套接字文件描述符
 * @param family 地址族（AF_INET/AF_INET6）
 * @param tos TOS 值（0-255）
 * @return 成功返回 0，失败返回 -1
 *
 * 设置 IP 数据包的服务类型字段，用于 QoS（服务质量）控制
 */
int set_raw_socket_tos(socket_t fd, int family, int tos);

/**
 * @brief 获取原始套接字的 TOS
 *
 * @param fd 套接字文件描述符
 * @param family 地址族（AF_INET/AF_INET6）
 * @return 成功返回 TOS 值，失败返回 -1
 */
int get_raw_socket_tos(socket_t fd, int family);

/**
 * @brief 设置原始套接字的 TTL（生存时间）
 *
 * @param fd 套接字文件描述符
 * @param family 地址族（AF_INET/AF_INET6）
 * @param ttl TTL 值（0-255）
 * @return 成功返回 0，失败返回 -1
 *
 * 设置 IP 数据包的 TTL 字段，控制数据包的跳数限制
 */
int set_raw_socket_ttl(socket_t fd, int family, int ttl);

/**
 * @brief 获取原始套接字的 TTL
 *
 * @param fd 套接字文件描述符
 * @param family 地址族（AF_INET/AF_INET6）
 * @return 成功返回 TTL 值，失败返回 -1
 */
int get_raw_socket_ttl(socket_t fd, int family);

/**
 * @brief 设置套接字缓冲区大小
 *
 * @param fd 套接字文件描述符
 * @param sz 期望设置的缓冲区大小（字节）
 * @return 成功返回 0，失败返回 -1
 *
 * 设置套接字的发送/接收缓冲区大小，用于优化网络性能
 */
int set_sock_buf_size(socket_t fd, int sz);

///////////////////////// socket bind //////////////////////////

/**
 * @brief 连接到指定地址
 *
 * @param fd 套接字文件描述符
 * @param addr 目标地址结构体
 * @param out_errno 输出错误码（可为 NULL）
 * @return 成功返回 0，失败返回 -1
 *
 * 发起 TCP/UDP 连接到指定目标地址
 */
int addr_connect(socket_t fd, const ioa_addr *addr, int *out_errno);

/**
 * @brief 绑定套接字到指定网络设备
 *
 * @param fd 套接字文件描述符
 * @param ifname 网络设备名称（如 "eth0"）
 * @return 成功返回 0，失败返回 -1
 *
 * 将套接字的数据流量限制在指定的网络接口上（仅 Linux/Unix）
 */
int sock_bind_to_device(socket_t fd, const unsigned char *ifname);

/**
 * @brief 绑定套接字到指定地址
 *
 * @param fd 套接字文件描述符
 * @param addr 绑定的地址结构体
 * @param reusable 是否可重用地址
 * @param debug 是否启用调试模式
 * @param st 套接字类型
 * @return 成功返回 0，失败返回 -1
 *
 * 将套接字绑定到本地指定地址和端口
 */
int addr_bind(socket_t fd, const ioa_addr *addr, int reusable, int debug, SOCKET_TYPE st);

/**
 * @brief 获取套接字绑定的本地地址
 *
 * @param fd 套接字文件描述符
 * @param addr 输出地址结构体
 * @return 成功返回 0，失败返回 -1
 *
 * 查询套接字当前绑定的本地地址和端口
 */
int addr_get_from_sock(socket_t fd, ioa_addr *addr);

/**
 * @brief 获取地址结构体的长度（字节数）
 * @param addr 地址结构体指针
 * @return 地址长度（字节）
 */
uint32_t get_ioa_addr_len(const ioa_addr *addr);

/**
 * @brief 设置地址的端口号
 * @param addr 地址结构体指针
 * @param port 端口号
 */
void addr_set_port(ioa_addr *addr, int port);

/**
 * @brief 获取地址的端口号
 * @param addr 地址结构体指针
 * @return 端口号
 */
int addr_get_port(const ioa_addr *addr);

///////////////////////// buffer read/write //////////////////////////
/**
 * @brief 读取并清空套接字缓冲区
 *
 * @param fd 套接字文件描述符
 *
 * 用于读取并丢弃套接字缓冲区中的剩余数据，防止数据残留影响后续操作
 */
void read_spare_buffer(socket_t);

////////////////////// socket creation/closing //////////////////////////

/**
 * @brief 创建套接字
 * @param domain 地址族（AF_INET/AF_INET6）
 * @param type 套接字类型（SOCK_STREAM/SOCK_DGRAM）
 * @param protocol 协议（IPPROTO_TCP/IPPROTO_UDP/0）
 * @return 成功返回套接字描述符，失败返回 -1
 */
socket_t socket_create(int domain, int type, int protocol);

/**
 * @brief 关闭套接字
 * @param fd 套接字文件描述符
 * @return 成功返回 0，失败返回 -1
 */
int socket_close(socket_t fd);

////////////////////// socket listen/accept //////////////////////////

/**
 * @brief 监听套接字
 * @param fd 套接字文件描述符
 * @param backlog 等待连接队列的最大长度
 * @return 成功返回 0，失败返回 -1
 */
int socket_listen(socket_t fd, int backlog);

/**
 * @brief 接受连接
 * @param fd 套接字文件描述符
 * @param client_addr 输出客户端地址（可为 NULL）
 * @return 成功返回新的套接字描述符，失败返回 -1
 */
socket_t socket_accept(socket_t fd, ioa_addr *client_addr);

////////////////////// socket read/write //////////////////////////

/**
 * @brief 发送数据
 * @param fd 套接字文件描述符
 * @param buf 发送缓冲区
 * @param len 发送数据长度
 * @param flags 标志位（如 MSG_NOSIGNAL）
 * @return 成功返回发送的字节数，失败返回 -1
 */
int socket_send(socket_t fd, const void *buf, size_t len, int flags);

/**
 * @brief 接收数据
 * @param fd 套接字文件描述符
 * @param buf 接收缓冲区
 * @param len 接收缓冲区大小
 * @param flags 标志位
 * @return 成功返回接收的字节数，失败返回 -1
 */
int socket_recv(socket_t fd, void *buf, size_t len, int flags);

/**
 * @brief 发送数据到指定地址
 * @param fd 套接字文件描述符
 * @param buf 发送缓冲区
 * @param len 发送数据长度
 * @param flags 标志位
 * @param addr 目标地址
 * @return 成功返回发送的字节数，失败返回 -1
 */
int socket_sendto(socket_t fd, const void *buf, size_t len, int flags, const ioa_addr *addr);

/**
 * @brief 接收数据并获取发送方地址
 * @param fd 套接字文件描述符
 * @param buf 接收缓冲区
 * @param len 接收缓冲区大小
 * @param flags 标志位
 * @param addr 输出发送方地址
 * @return 成功返回接收的字节数，失败返回 -1
 */
int socket_recvfrom(socket_t fd, void *buf, size_t len, int flags, ioa_addr *addr);

////////////////////// socket options //////////////////////////

/**
 * @brief 设置发送超时
 * @param fd 套接字文件描述符
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功返回 0，失败返回 -1
 */
int socket_set_send_timeout(socket_t fd, int timeout_ms);

/**
 * @brief 设置接收超时
 * @param fd 套接字文件描述符
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功返回 0，失败返回 -1
 */
int socket_set_recv_timeout(socket_t fd, int timeout_ms);

/**
 * @brief 获取发送超时
 * @param fd 套接字文件描述符
 * @param timeout_ms 输出超时时间（毫秒）
 * @return 成功返回 0，失败返回 -1
 */
int socket_get_send_timeout(socket_t fd, int *timeout_ms);

/**
 * @brief 获取接收超时
 * @param fd 套接字文件描述符
 * @param timeout_ms 输出超时时间（毫秒）
 * @return 成功返回 0，失败返回 -1
 */
int socket_get_recv_timeout(socket_t fd, int *timeout_ms);

/**
 * @brief 设置广播选项
 * @param fd 套接字文件描述符
 * @param enable 是否启用（1=启用，0=禁用）
 * @return 成功返回 0，失败返回 -1
 */
int socket_set_broadcast(socket_t fd, int enable);

/**
 * @brief 获取广播选项状态
 * @param fd 套接字文件描述符
 * @param enabled 输出是否启用
 * @return 成功返回 0，失败返回 -1
 */
int socket_get_broadcast(socket_t fd, int *enabled);

/**
 * @brief 设置TCP_NODELAY（禁用Nagle算法）
 * @param fd 套接字文件描述符
 * @param enable 是否启用（1=启用，0=禁用）
 * @return 成功返回 0，失败返回 -1
 */
int socket_tcp_set_nodelay(socket_t fd, int enable);

/**
 * @brief 获取TCP_NODELAY状态
 * @param fd 套接字文件描述符
 * @param enabled 输出是否启用
 * @return 成功返回 0，失败返回 -1
 */
int socket_tcp_get_nodelay(socket_t fd, int *enabled);

/**
 * @brief 设置TCP keepalive参数
 * @param fd 套接字文件描述符
 * @param idle 空闲时间（秒）
 * @param interval 重试间隔（秒）
 * @param count 重试次数
 * @return 成功返回 0，失败返回 -1
 */
int socket_tcp_set_keepalive_params(socket_t fd, int idle, int interval, int count);

/**
 * @brief 设置LINGER选项
 * @param fd 套接字文件描述符
 * @param onoff 是否启用（1=启用，0=禁用）
 * @param linger_seconds 延迟关闭时间（秒）
 * @return 成功返回 0，失败返回 -1
 */
int socket_set_linger(socket_t fd, int onoff, int linger_seconds);

#if defined(SO_REUSEPORT)
/**
 * @brief 设置端口复用
 * @param fd 套接字文件描述符
 * @param enable 是否启用（1=启用，0=禁用）
 * @return 成功返回 0，失败返回 -1
 */
int socket_set_reuseport(socket_t fd, int enable);
#endif

//////////////////// socket error handle ////////////////////
/**
 * @brief 处理套接字错误，返回是否可忽略
 *
 * @return true 表示错误可忽略（如 EAGAIN/EWOULDBLOCK），false 表示不可忽略
 *
 * 检查套接字错误码，判断是否为非致命错误
 */
bool handle_socket_error(void);

/**
 * @brief 获取 socket 错误码
 * @return 错误码
 */
int socket_errno(void);

/**
 * @brief 检查是否为权限错误
 * @return true 表示权限错误，否则返回 false
 */
bool socket_eperm(void);

/**
 * @brief 检查是否为内存不足错误
 * @return true 表示内存不足，否则返回 false
 */
bool socket_enomem(void);

/**
 * @brief 检查是否为中断错误
 * @return true 表示被中断，否则返回 false
 */
bool socket_eintr(void);

/**
 * @brief 检查是否为无效文件描述符错误
 * @return true 表示无效文件描述符，否则返回 false
 */
bool socket_ebadf(void);

/**
 * @brief 检查是否为权限拒绝错误
 * @return true 表示权限拒绝，否则返回 false
 */
bool socket_eacces(void);

/**
 * @brief 检查是否为缓冲区不足错误
 * @return true 表示缓冲区不足，否则返回 false
 */
bool socket_enobufs(void);

/**
 * @brief 检查是否为重试错误
 * @return true 表示需要重试，否则返回 false
 */
bool socket_eagain(void);

/**
 * @brief 检查是否为阻塞错误
 * @return true 表示操作阻塞，否则返回 false
 */
bool socket_ewouldblock(void);

/**
 * @brief 检查是否为进行中错误
 * @return true 表示操作进行中，否则返回 false
 */
bool socket_einprogress(void);

/**
 * @brief 检查是否为连接重置错误
 * @return true 表示连接重置，否则返回 false
 */
bool socket_econnreset(void);

/**
 * @brief 检查是否为连接拒绝错误
 * @return true 表示连接拒绝，否则返回 false
 */
bool socket_econnrefused(void);

/**
 * @brief 检查是否为主机宕机错误
 * @return true 表示主机宕机，否则返回 false
 */
bool socket_ehostdown(void);
/**
 * @brief 检查是否为消息过大错误
 * @return true 表示消息过大，否则返回 false
 */
bool socket_emsgsize(void);

#endif // SOCKET_H

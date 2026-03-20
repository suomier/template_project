#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 安全的字符串复制宏
 * @param dst 目标字符串
 * @param src 源字符串
 *
 * 根据目标类型自动选择 strcpy 或 strncpy，防止缓冲区溢出
 */
// NOLINTBEGIN(clang-diagnostic-string-compare)
#define STRCPY(dst, src)                                            \
    do                                                              \
    {                                                               \
        if ((const char *)(dst) != (const char *)(src))             \
        {                                                           \
            if (sizeof(dst) == sizeof(char *))                      \
                strcpy(((char *)(dst)), (const char *)(src));       \
            else                                                    \
            {                                                       \
                size_t szdst = sizeof((dst));                       \
                strncpy((char *)(dst), (const char *)(src), szdst); \
                ((char *)(dst))[szdst - 1] = 0;                     \
            }                                                       \
        }                                                           \
    } while (0)
// NOLINTEND(clang-diagnostic-string-compare)

#endif // STRINGUTILS_H

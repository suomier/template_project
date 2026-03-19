#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

// ============================================ DP 指针

// 在公共类中声明私有成员指针和访问函数（通常放在 private 区域）
#define Q_DECLARE_PRIVATE(Class)                                                                            \
    Class##Private *d_ptr = nullptr;                                                                        \
    inline Class##Private *d_func() { return reinterpret_cast<Class##Private *>(d_ptr); }                   \
    inline const Class##Private *d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr); } \
    friend class Class##Private;

// 在私有类中声明公共类指针和访问函数（用于私有类回调公共类）
#define Q_DECLARE_PUBLIC(Class)                                                      \
    Class *const q_ptr;                                                              \
    inline Class *q_func() { return static_cast<Class *>(q_ptr); }                   \
    inline const Class *q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define Q_D(Class) Class##Private *const d = d_func()
#define Q_CD(Class) const Class##Private *const d = d_func()

#define Q_Q(Class) Class *const q = q_func()
#define Q_CQ(Class) const Class *const q = q_func()

#endif // COMMON_DEF_H

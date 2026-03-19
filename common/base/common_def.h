#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

// ============================================ DP 指针

// 在公共类中声明私有成员指针和访问函数（通常放在 private 区域）
// 使用 std::unique_ptr 自动管理内存，避免手动 delete
#define DECLARE_PRIVATE(Class)                                          \
    std::unique_ptr<Class##Private> d_ptr;                              \
    inline Class##Private *d_func() { return d_ptr.get(); }             \
    inline const Class##Private *d_func() const { return d_ptr.get(); } \
    friend class Class##Private;

// 在私有类中声明公共类指针和访问函数（用于私有类回调公共类）
#define DECLARE_PUBLIC(Class)                                                        \
    Class *const q_ptr;                                                              \
    inline Class *q_func() { return static_cast<Class *>(q_ptr); }                   \
    inline const Class *q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define DP_D(Class) Class##Private *const d = d_func()
#define DP_CD(Class) const Class##Private *const d = d_func()

#define DP_Q(Class) Class *const q = q_func()
#define DP_CQ(Class) const Class *const q = q_func()

// ============================================== 禁止拷贝和移动构造
#define DISABLE_COPY(Class)        \
    Class(const Class &) = delete; \
    Class &operator=(const Class &) = delete;

#define DISABLE_MOVE(Class)   \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define DISABLE_COPY_MOVE(Class) \
    DISABLE_COPY(Class)          \
    DISABLE_MOVE(Class)

#endif // COMMON_DEF_H

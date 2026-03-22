/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_FUNCTION_VIEW_H_
#define API_FUNCTION_VIEW_H_

#include <type_traits>
#include <utility>

// 源代码来自 webrtc M96版本, 路径: function_view.h
// FunctionView 与 std::function 类似，会包装任何可调用对象并隐藏其
// 实际类型，只暴露其签名。但与 std::function 不同的是，
// FunctionView 不拥有其可调用对象——它只是指向它。因此，它主要
// 适合作为函数参数使用，当可调用参数在函数返回后不会被再次调用时。
//
// 它的构造函数是隐式的，这样调用者就不必将 lambda 表达式和
// 其他可调用对象显式地转换为 FunctionView<Blah(Blah, Blah)>。这是
// 安全的，因为 FunctionView 只是对真实可调用对象的引用。
//
// 使用示例：
//
//   void SomeFunction(rtc::FunctionView<int(int)> index_transform);
//   ...
//   SomeFunction([](int i) { return 2 * i + 1; });
//
// 注意：FunctionView 很小（本质上只有两个指针）并且可平凡复制，
// 所以按值传递可能比按 const 引用传递更便宜。

template <typename T>
class FunctionView;

/**
 * @brief 函数视图类模板，提供对可调用对象的非拥有视图
 * @tparam RetT 返回类型
 * @tparam ArgT 参数类型包
 *
 * @details
 * FunctionView 是一个轻量级的函数包装器，类似于 std::function，但不拥有可调用对象。
 * 它只存储指向可调用对象的指针，因此：
 * - 不会进行任何内存分配
 * - 大小等于一个指针（高效）
 * - 生命周期：被包装的对象必须在 FunctionView 使用期间保持有效
 * - 类似于 string_view 和 std::string 的关系
 *
 * @warning
 * 不要存储 FunctionView 或在对象销毁后使用它，这会导致未定义行为。
 */
template <typename RetT, typename... ArgT>
class FunctionView<RetT(ArgT...)> final
{
public:
    /**
     * @brief 接受 lambda 和其他可调用对象的构造函数
     * @tparam F 可调用对象类型
     * @param f 要包装的可调用对象（使用完美转发）
     *
     * @details
     * 该构造函数接受除以下类型外的所有可调用对象：
     * - 函数指针（有专门的构造函数处理）
     * - nullptr（有专门的构造函数处理）
     * - FunctionView 对象（使用隐式声明的拷贝构造函数）
     *
     * @note
     * 该构造函数使用 SFINAE 技术来限制模板参数类型，确保不会与其他构造函数冲突。
     */
    template <typename F,
              typename std::enable_if<
                  !std::is_function<typename std::remove_pointer<typename std::remove_reference<F>::type>::type>::value         // 排除函数指针类型
                  && !std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value                                     // 排除 nullptr 类型
                  && !std::is_same<FunctionView, typename std::remove_cv<typename std::remove_reference<F>::type>::type>::value // FunctionView (让拷贝构造函数处理)
                  >::type * = nullptr>
    FunctionView(F &&f)
        : call_(CallVoidPtr<typename std::remove_reference<F>::type>)
    {
        f_.void_ptr = &f;
    }

    /**
     * @brief 接受函数指针的构造函数
     * @tparam F 函数指针类型
     * @param f 要包装的函数指针（使用完美转发）
     *
     * @details
     * 如果传入的函数指针为 null，则创建一个空的 FunctionView。
     */
    template <typename F,
              typename std::enable_if<std::is_function<typename std::remove_pointer<
                  typename std::remove_reference<F>::type>::type>::value>::type * = nullptr>
    FunctionView(F &&f)
        : call_(f ? CallFunPtr<typename std::remove_pointer<F>::type> : nullptr)
    {
        f_.fun_ptr = reinterpret_cast<void (*)()>(f);
    }

    /**
     * @brief 接受 nullptr 的构造函数
     * @tparam F nullptr_t 类型
     * @param f nullptr 值
     *
     * @details
     * 创建一个空的 FunctionView 对象。
     */
    template <typename F,
              typename std::enable_if<std::is_same<
                  std::nullptr_t, typename std::remove_cv<F>::type //
                  >::value>::type * = nullptr>
    FunctionView(F &&f) : call_(nullptr)
    {
    }

    /**
     * @brief 默认构造函数
     *
     * @details
     * 创建一个空的 FunctionView 对象。
     */
    FunctionView()
        : call_(nullptr)
    {
    }

    /**
     * @brief 函数调用运算符
     * @param args 传递给包装函数的参数（使用完美转发）
     * @return RetT 被包装函数的返回值
     *
     * @details
     * 调用存储的可调用对象，并传递所有参数。
     * 要求 FunctionView 必须包含有效的可调用对象（非空）。
     *
     * @pre
     * FunctionView 必须非空（即 call_ 不为 nullptr），否则会触发断言失败。
     */
    RetT operator()(ArgT... args) const
    {
        RTC_DCHECK(call_);
        return call_(f_, std::forward<ArgT>(args)...);
    }

    /**
     * @brief bool 转换运算符
     * @return true 如果 FunctionView 包含有效的可调用对象
     * @return false 如果 FunctionView 为空（null）
     *
     * @details
     * 显式转换运算符，用于检查 FunctionView 是否包含有效的可调用对象。
     * 等价于检查 call_ 是否为 nullptr。
     */
    explicit operator bool() const
    {
        // 等价于 call_ != nullptr
        return !!call_;
    }

private:
    /**
     * @brief 用于存储可调用对象的联合体
     *
     * @details
     * 联合体用于存储不同类型的可调用对象指针：
     * - void_ptr: 指向 lambda、仿函数或其他可调用对象的指针
     * - fun_ptr: 指向函数的指针
     *
     * 使用联合体是因为需要根据可调用对象的类型使用不同的存储方式。
     */
    union VoidUnion
    {
        void *void_ptr;    ///< 指向可调用对象的通用指针
        void (*fun_ptr)(); ///< 函数指针
    };

    /**
     * @brief 调用存储在 void_ptr 中的可调用对象
     * @tparam F 可调用对象的类型
     * @param vu 包含可调用对象指针的联合体
     * @param args 传递给可调用对象的参数（使用完美转发）
     * @return RetT 可调用对象的返回值
     *
     * @details
     * 这是一个静态模板函数，用于类型擦除后的回调。
     * 它将存储在联合体中的指针转换回原始类型，并调用该对象。
     */
    template <typename F>
    static RetT CallVoidPtr(VoidUnion vu, ArgT... args)
    {
        return (*static_cast<F *>(vu.void_ptr))(std::forward<ArgT>(args)...);
    }

    /**
     * @brief 调用存储在 fun_ptr 中的函数指针
     * @tparam F 函数类型
     * @param vu 包含函数指针的联合体
     * @param args 传递给函数的参数（使用完美转发）
     * @return RetT 函数的返回值
     *
     * @details
     * 这是一个静态模板函数，用于调用函数指针类型的可调用对象。
     * 它将存储在联合体中的函数指针转换回原始函数类型，并调用该函数。
     */
    template <typename F>
    static RetT CallFunPtr(VoidUnion vu, ArgT... args)
    {
        return (reinterpret_cast<typename std::add_pointer<F>::type>(vu.fun_ptr))(std::forward<ArgT>(args)...);
    }

    /**
     * @brief 指向可调用对象的指针（类型已擦除）
     *
     * @details
     * 使用联合体存储可调用对象的指针，实现了类型擦除。
     * 具体的类型信息保存在 call_ 函数指针中。
     * - 如果可调用对象是函数指针，使用 fun_ptr 成员
     * - 如果可调用对象是 lambda 或其他对象，使用 void_ptr 成员
     */
    VoidUnion f_;

    /**
     * @brief 指向分发函数的指针
     *
     * @details
     * 这是一个函数指针，用于调用存储在 f_ 中的可调用对象。
     * 该分发函数知道 f_ 中存储的具体类型，以及如何调用它。
     *
     * FunctionView 对象为空（null）当且仅当 call_ 为 nullptr。
     * 这种设计类似于虚函数表，但更轻量级。
     */
    RetT (*call_)(VoidUnion, ArgT...);
};

#endif // API_FUNCTION_VIEW_H_

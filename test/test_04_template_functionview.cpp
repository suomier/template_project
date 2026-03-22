/*
 * SFINAE (Substitution Failure Is Not An Error) 技术演示
 *
 * 本文件演示了 FunctionView 构造函数中使用的 SFINAE 语法
 * 从简单到复杂逐步解释各个部分
 */

#include <gtest/gtest.h>

#include <string>
#include <type_traits>

// ============================================
// 第1部分：最基本的模板构造函数
// ============================================
class Example1
{
public:
    // 最简单的模板构造函数
    template <typename F>
    Example1(F &&f)
    {
        // 无操作，仅用于编译测试
        (void)f;
    }
};

TEST(SFINAE, BasicTemplateConstructor)
{
    Example1 e1([]() {}); // lambda
    Example1 e2(42);      // int
    Example1 e3("hello"); // const char*
    SUCCEED();
}

// ============================================
// 第2部分：std::enable_if 的作用
// ============================================
class Example2
{
public:
    // 只在条件为 true 时，这个构造函数才存在
    // 如果条件为 false，SFINAE 会忽略这个构造函数
    //
    // std::is_integral<F>::value 为 true 的类型：
    // - bool, char, wchar_t, char8_t, char16_t, char32_t
    // - short, int, long, long long
    // - unsigned short, unsigned int, unsigned long, unsigned long long
    //
    // 使用这个构造函数的例子：
    // - Example2(42)      → int ✓
    // - Example2(42LL)    → long long ✓
    // - Example2('a')     → char ✓
    // - Example2(true)    → bool ✓
    // - Example2(42U)     → unsigned int ✓
    template <typename F,
              typename std::enable_if<std::is_integral<F>::value>::type * = nullptr>
    Example2(F &&f)
        : value_(f)
    {
    }

    // 接受所有其他类型的构造函数
    //
    // std::is_integral<F>::value 为 false 的类型：
    // - float, double, long double
    // - 指针类型（int*, void*, std::string* 等）
    // - 类类型（std::string, std::vector, lambda 等）
    // - 枚举类型
    // - 数组类型
    //
    // 使用这个构造函数的例子：
    // - Example2(3.14)       → double ✓
    // - Example2("hello")    → const char* ✓
    // - Example2(std::string()) → std::string ✓
    // - Example2([](){})     → lambda ✓
    template <typename F,
              typename std::enable_if<!std::is_integral<F>::value>::type * = nullptr>
    Example2(F &&f)
        : value_(0)
    {
        (void)f;
    }

    int value() const { return value_; }

private:
    int value_;
};

TEST(SFINAE, EnableIf)
{
    Example2 e1(42); // 使用第一个构造函数
    EXPECT_EQ(e1.value(), 42);

    Example2 e2(3.14); // 使用第二个构造函数
    EXPECT_EQ(e2.value(), 0);

    Example2 e3("hello"); // 使用第二个构造函数
    EXPECT_EQ(e3.value(), 0);
}

// ============================================
// 第3部分：std::remove_reference 的作用
// ============================================
class Example3
{
public:
    // std::remove_reference<F>::type：去除引用类型
    //
    // 工作原理：
    // - std::remove_reference<int>::type        → int
    // - std::remove_reference<int&>::type       → int
    // - std::remove_reference<int&&>::type      → int
    // - std::remove_reference<const int&>::type → const int（只去引用，不去 const）
    //
    // F&& 是万能引用（forwarding reference）：
    // - 如果传入左值，F 推导为左值引用（例如：int&）
    // - 如果传入右值，F 推导为非引用类型（例如：int）
    //
    // 使用 std::remove_reference 的目的：
    // - 无论传入左值还是右值，都去除引用得到原始类型
    // - 然后用 std::is_same 检查是否为 int
    template <typename F,
              typename std::enable_if<
                  std::is_same<int, typename std::remove_reference<F>::type>::value // 去除引用
                  >::type * = nullptr>
    Example3(F &&f)
        : value_(f)
    {
    }

    int value() const
    {
        return value_;
    }

private:
    int value_;
};

TEST(SFINAE, RemoveReference)
{
    int x = 10;
    Example3 e1(42); // T = int, F = int
    EXPECT_EQ(e1.value(), 42);

    Example3 e2(x); // T = int&, F = int&
    EXPECT_EQ(e2.value(), 10);

    Example3 e3(std::move(x)); // T = int&&, F = int&&
    EXPECT_EQ(e3.value(), 10);
}

// ============================================
// 第4部分：std::remove_cv 的作用
// ============================================
class Example4
{
public:
    // std::remove_cv<F>::type：去除 const 和 volatile
    //
    // 组合使用 std::remove_reference 和 std::remove_cv 的效果：
    //
    // 去除引用：
    // - std::remove_reference<int>::type        → int
    // - std::remove_reference<int&>::type       → int
    // - std::remove_reference<int&&>::type      → int
    // - std::remove_reference<const int&>::type → const int
    //
    // 去除 const/volatile：
    // - std::remove_cv<int>::type           → int
    // - std::remove_cv<const int>::type     → int
    // - std::remove_cv<volatile int>::type  → int
    // - std::remove_cv<const volatile int>::type → int
    //
    // 组合处理具体例子：
    // - int              → remove_reference → int           → remove_cv → int ✓
    // - int&             → remove_reference → int           → remove_cv → int ✓
    // - int&&            → remove_reference → int           → remove_cv → int ✓
    // - const int&       → remove_reference → const int     → remove_cv → int ✓
    // - volatile int&    → remove_reference → volatile int  → remove_cv → int ✓
    // - const int        → remove_reference → const int     → remove_cv → int ✓
    //
    // 这样可以匹配所有形式的 int 类型，无论是否是 const、volatile 或引用
    template <typename F,
              typename std::enable_if<
                  std::is_same<int, typename std::remove_cv<
                                        typename std::remove_reference<F>::type // 去除引用
                                        >::type>::value                         // 去除 const 和 volatile
                  >::type * = nullptr>                                          //
    Example4(F &&f)
        : value_(f)
    {
    }

    int value() const { return value_; }

private:
    int value_;
};

TEST(SFINAE, RemoveCV)
{
    int x = 10;
    const int y = 20;
    volatile int z = 30;

    Example4 e1(42); // int
    EXPECT_EQ(e1.value(), 42);

    Example4 e2(x); // int&
    EXPECT_EQ(e2.value(), 10);

    Example4 e3(y); // const int&
    EXPECT_EQ(e3.value(), 20);

    Example4 e4(z); // volatile int&
    EXPECT_EQ(e4.value(), 30);
}

// ============================================
// 第5部分：组合多个条件（逻辑与）
// ============================================
class Example5
{
public:
    // 同时满足多个条件时，构造函数才存在
    //
    // 条件 1：std::is_integral<F>::value（必须是整数类型）
    // - bool, char, wchar_t, char8_t, char16_t, char32_t
    // - short, int, long, long long
    // - unsigned short, unsigned int, unsigned long, unsigned long long
    //
    // 条件 2：std::is_signed<F>::value（必须是有符号类型）
    // - 有符号类型可以表示负数
    // - 包括：signed char, short, int, long, long long
    //
    // 组合条件：必须同时是整数类型 AND 有符号类型
    //
    // 满足条件的类型：
    // - signed char (char 在某些平台上也可能是有符号的)
    // - short ✓
    // - int ✓
    // - long ✓
    // - long long ✓
    //
    // 不满足条件的类型：
    // - bool (虽然 is_integral=true，但不是有符号类型)
    // - unsigned short (is_integral=true, 但 is_signed=false)
    // - unsigned int (is_integral=true, 但 is_signed=false)
    // - unsigned long (is_integral=true, 但 is_signed=false)
    // - unsigned long long (is_integral=true, 但 is_signed=false)
    // - float, double (is_integral=false)
    //
    // 使用这个构造函数的例子：
    // - Example5(42)      → int ✓ (is_integral=true, is_signed=true)
    // - Example5(-10)     → int ✓
    // - Example5(42LL)    → long long ✓
    // - Example5('a')     → char ✓ (假设 char 是有符号的)
    // - Example5(42U)     ✗ (unsigned int: is_integral=true, 但 is_signed=false)
    // - Example5(3.14)    ✗ (double: is_integral=false)
    template <typename F,
              typename std::enable_if<
                  std::is_integral<F>::value && std::is_signed<F>::value //
                  >::type * = nullptr>
    Example5(F &&f)
        : value_(f)
    {
    }

    int value() const { return value_; }

private:
    int value_;
};

TEST(SFINAE, MultipleConditions)
{
    Example5 e1(42); // int：是有符号整数 ✓
    EXPECT_EQ(e1.value(), 42);

    Example5 e2(-10); // int：是有符号整数 ✓
    EXPECT_EQ(e2.value(), -10);
}

// ============================================
// 第6部分：排除特定类型（逻辑非）
// ============================================
class Example6
{
public:
    // 排除 nullptr 类型
    //
    // std::nullptr_t：C++11 引入的空指针字面量的类型
    // - nullptr 的类型就是 std::nullptr_t
    // - 可以隐式转换为任何指针类型或成员指针
    //
    // std::remove_cv<F>::type：去除 const 和 volatile
    // - std::remove_cv<std::nullptr_t>::type → std::nullptr_t
    // - std::remove_cv<const std::nullptr_t>::type → std::nullptr_t
    //
    // std::is_same<T, U>::value：检查两个类型是否相同
    // - std::is_same<int, int>::value → true
    // - std::is_same<int, float>::value → false
    //
    // 条件：!std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value
    // - 如果 F 是 std::nullptr_t 或 const/volatile std::nullptr_t，条件为 false
    // - 如果 F 是其他类型，条件为 true
    //
    // 使用这个模板构造函数的例子（条件为 true）：
    // - Example6(42)           → int ✓ (不是 nullptr_t)
    // - Example6(3.14)         → double ✓
    // - Example6("hello")      → const char* ✓
    // - Example6(nullptr)     ✗ (是 nullptr_t，条件为 false)
    // - Example6((int*)nullptr) → int* ✓ (指针类型，不是 nullptr_t)
    //
    // 注意：即使传入的是指针类型的 nullptr（如 (int*)nullptr），也不会被排除
    // 因为类型是 int* 而不是 std::nullptr_t
    template <typename F,
              typename std::enable_if<
                  !std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value //
                  >::type * = nullptr>
    Example6(F &&f)
        : is_nullptr_(false)
    {
        (void)f;
    }

    // 专门处理 nullptr
    //
    // 非模板构造函数，精确匹配 std::nullptr_t 类型
    // - 当传入 nullptr 时，优先使用这个构造函数（精确匹配优先于模板）
    // - 与上面的模板构造函数形成互补，确保所有情况都有对应的处理
    //
    // 使用这个构造函数的例子：
    // - Example6(nullptr) → std::nullptr_t ✓ (使用这个非模板构造函数)
    Example6(std::nullptr_t)
        : is_nullptr_(true)
    {
    }

    bool is_nullptr() const { return is_nullptr_; }

private:
    bool is_nullptr_;
};

TEST(SFINAE, ExcludeNullptr)
{
    Example6 e1(42); // 第一个构造函数
    EXPECT_FALSE(e1.is_nullptr());

    Example6 e2("hello"); // 第一个构造函数
    EXPECT_FALSE(e2.is_nullptr());

    Example6 e3(nullptr); // 第二个构造函数
    EXPECT_TRUE(e3.is_nullptr());
}

// ============================================
// 第7部分：std::is_function 和 std::is_pointer
// ============================================
class Example7
{
public:
    // std::is_function<T>::value：检查 T 是否是函数类型（非函数指针）
    //
    // 函数类型 vs 函数指针类型的区别：
    // - 函数类型：void(int)，即函数的签名
    // - 函数指针类型：void (*)(int)，即指向函数的指针
    //
    // std::is_function 的判断结果：
    // - std::is_function<void(int)>::value → true ✓ (函数类型)
    // - std::is_function<void (*)(int)>::value → false ✗ (函数指针类型)
    // - std::is_function<int>::value → false ✗
    //
    // std::remove_pointer<T>::type：去除指针
    // - std::remove_pointer<int*>::type → int
    // - std::remove_pointer<void (*)(int)>::type → void(int)
    // - std::remove_pointer<int>::type → int
    //
    // 复杂表达式分解：
    // 1. F：传入的类型
    // 2. std::remove_reference<F>::type：去除引用
    // 3. std::remove_pointer<...>::type：去除指针
    // 4. std::is_function<...>::value：检查是否是函数类型
    // 5. !：取反，排除函数指针类型
    //
    // 排除函数指针类型的模板构造函数
    // 条件：!std::is_function<typename std::remove_pointer<typename std::remove_reference<F>::type>::type>::value
    // - 如果 F 是函数指针，去除指针后是函数类型，is_function 为 true，条件为 false（被排除）
    // - 如果 F 是其他类型，is_function 为 false，条件为 true（接受）
    //
    // 使用这个构造函数的例子（条件为 true）：
    // - Example7([]() {})           → lambda ✓ (不是函数指针)
    // - Example7(42)                → int ✓
    // - Example7("hello")           → const char* ✓ (普通指针，不是函数指针)
    // - Example7(3.14)              → double ✓
    // - Example7(std::string())     → std::string ✓
    //
    // 不使用这个构造函数的例子（条件为 false，使用下面的函数指针构造函数）：
    // - Example7(&someFunction)     → void(*)() ✗ (函数指针)
    // - Example7(funcPtr)           → int(*)(int) ✗ (函数指针)
    template <typename F,
              typename std::enable_if<
                  !std::is_function<typename std::remove_pointer< // 去除指针
                      typename std::remove_reference<F>::type     // 去除引用
                      >::type>::value                             // 判断是否是函数类型
                  >::type * = nullptr>
    Example7(F &&f)
        : is_func_ptr_(false)
    {
        (void)f;
    }

    // 专门处理函数指针
    //
    // Ret (*f)(Args...)：标准的函数指针语法
    // - Ret：返回值类型
    // - Args...：参数类型列表
    // - *f：指针
    //
    // 这是一个非 SFINAE 的模板构造函数，精确匹配函数指针类型
    // - 当传入函数指针时，使用这个构造函数
    // - 与上面的 SFINAE 构造函数形成互补
    //
    // 使用这个构造函数的例子：
    // - void func() {} Example7 e(&func) → void(*)() ✓
    // - int add(int a, int b) { return a + b; } Example7 e(&add) → int(*)(int, int) ✓
    //
    // 实际应用：
    // 这个设计模式常用于区分可调用对象：
    // - lambda、函数对象：使用模板构造函数
    // - 函数指针：使用专门的函数指针构造函数
    // - 可以根据不同的可调用类型进行不同的处理
    template <typename Ret, typename... Args>
    Example7(Ret (*f)(Args...))
        : is_func_ptr_(true)
    {
        (void)f;
    }

    bool is_func_ptr() const { return is_func_ptr_; }

private:
    bool is_func_ptr_;
};

TEST(SFINAE, IsFunctionAndPointer)
{
    Example7 e1([]() {}); // lambda
    EXPECT_FALSE(e1.is_func_ptr());

    Example7 e2(42); // int
    EXPECT_FALSE(e2.is_func_ptr());
}

// ============================================
// 第8部分：完整的 FunctionView 构造函数逻辑
// ============================================
// 模拟 FunctionView 的实现
//
// FunctionView 是一个轻量级的函数包装器，可以持有任何可调用对象
// 它使用 SFINAE 技术来区分不同类型的输入，并选择合适的构造函数
//
// 设计目标：
// 1. 接受 lambda 表达式、函数对象等可调用对象
// 2. 专门处理 nullptr（表示空函数）
// 3. 正确处理拷贝构造
// 4. 排除函数指针（通常不推荐直接使用函数指针）
class FunctionView
{
public:
    enum class Type
    {
        Callable, // 可调用对象（lambda、函数对象等）
        Nullptr,  // nullptr 表示空函数
        Copy      // 从另一个 FunctionView 拷贝构造
    };

    // 主模板构造函数：接受可调用对象
    //
    // SFINAE 条件分解（三个条件使用 && 连接，必须同时满足）：
    //
    // 条件 1：!std::is_function<typename std::remove_pointer<typename std::remove_reference<F>::type>::type>::value
    // - 目的：排除函数指针类型
    // - std::remove_reference<F>::type：去除引用（处理 F& 或 F&&）
    // - std::remove_pointer<...>::type：去除指针（将函数指针转为函数类型）
    // - std::is_function<...>::value：检查是否是函数类型
    // - !：取反，如果不是函数指针，条件为 true
    //
    // 条件 2：!std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value
    // - 目的：排除 nullptr 类型
    // - std::remove_cv<F>::type：去除 const 和 volatile
    // - std::is_same<std::nullptr_t, ...>::value：检查是否是 nullptr_t
    // - !：取反，如果不是 nullptr_t，条件为 true
    //
    // 条件 3：!std::is_same<FunctionView, typename std::remove_cv<typename std::remove_reference<F>::type>::type>::value
    // - 目的：排除 FunctionView 自身（让拷贝构造函数处理）
    // - std::remove_reference<F>::type：去除引用
    // - std::remove_cv<...>::type：去除 const 和 volatile
    // - std::is_same<FunctionView, ...>::value：检查是否是 FunctionView
    // - !：取反，如果不是 FunctionView，条件为 true
    //
    // 使用这个构造函数的例子（三个条件同时满足）：
    // - FunctionView([]() {})           → lambda ✓ (非函数指针、非 nullptr、非 FunctionView)
    // - FunctionView([](int x){})       → lambda with param ✓
    // - FunctionView(Functor())         → 函数对象 ✓
    // - FunctionView(std::function<void()>{}) → std::function ✓
    // - FunctionView(42)                → int (虽然不是可调用对象，但满足条件)
    //
    // 不使用这个构造函数的例子（至少有一个条件不满足）：
    // - FunctionView(nullptr)          → nullptr_t ✗ (条件 2 不满足)
    // - FunctionView(fv1)              → FunctionView ✗ (条件 3 不满足)
    // - FunctionView(const FunctionView&) → const FunctionView& ✗ (条件 3 不满足)
    template <typename F,
              typename std::enable_if<
                  !std::is_function<typename std::remove_pointer<typename std::remove_reference<F>::type>::type>::value  // 排除函数指针类型
                  && !std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value                              // 排除 nullptr 类型
                  && !std::is_same<FunctionView, typename std::remove_cv<typename std::remove_reference<F>::type>::type> // FunctionView (让拷贝构造函数处理)
                     ::value>::type * = nullptr>
    FunctionView(F &&f)
        : type_(Type::Callable)
    {
        (void)f;
    }

    // nullptr 构造函数：专门处理 nullptr
    //
    // 非模板构造函数，精确匹配 std::nullptr_t 类型
    // - 当传入 nullptr 时，使用这个构造函数
    // - 用于表示空函数或未初始化的函数
    // - 与主模板构造函数的条件 2 形成互补
    //
    // 使用这个构造函数的例子：
    // - FunctionView(nullptr) → std::nullptr_t ✓
    //
    // 注意：这个构造函数优先于模板构造函数，因为它是精确匹配
    FunctionView(std::nullptr_t)
        : type_(Type::Nullptr)
    {
    }

    // 拷贝构造函数：处理 FunctionView 类型的拷贝
    //
    // 非模板构造函数，精确匹配 const FunctionView&
    // - 当传入另一个 FunctionView 对象时，使用这个构造函数
    // - 这也隐式禁用了移动构造函数（如果需要需要显式声明）
    // - 与主模板构造函数的条件 3 形成互补
    //
    // 使用这个构造函数的例子：
    // - FunctionView fv1([](){});
    // - FunctionView fv2(fv1);  → 使用拷贝构造函数 ✓
    // - const FunctionView fv3 = fv1;  → 使用拷贝构造函数 ✓
    FunctionView(const FunctionView &)
        : type_(Type::Copy)
    {
    }

    Type type() const
    {
        return type_;
    }

private:
    Type type_;
};

TEST(SFINAE, FunctionViewLogic)
{
    // 测试 1：lambda 表达式
    FunctionView fv1([]() {});
    EXPECT_EQ(fv1.type(), FunctionView::Type::Callable);

    // 测试 2：函数对象
    struct Functor
    {
        void operator()() const {}
    };
    Functor functor;
    FunctionView fv2(functor);
    EXPECT_EQ(fv2.type(), FunctionView::Type::Callable);

    // 测试 3：nullptr
    FunctionView fv3(nullptr);
    EXPECT_EQ(fv3.type(), FunctionView::Type::Nullptr);

    // 测试 4：拷贝构造
    FunctionView fv4(fv1);
    EXPECT_EQ(fv4.type(), FunctionView::Type::Copy);

    // 测试 5：const FunctionView（使用拷贝构造函数）
    const FunctionView fv5 = fv1;
    EXPECT_EQ(fv5.type(), FunctionView::Type::Copy);
}

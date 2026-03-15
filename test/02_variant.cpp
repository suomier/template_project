#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

// std::variant 是 C++17 引入的类型安全联合体（type-safe union）
// 它可以在同一时间持有指定类型中的任意一种类型的值

// 示例1: 基本用法
void basic_usage()
{
    std::cout << "=== 基本用法 ===" << std::endl;

    // 定义一个 variant，可以存储 int, double, 或 string
    std::variant<int, double, std::string> var;

    // 存储并获取不同类型的值
    var = 42; // 存储int
    std::cout << "int: " << std::get<int>(var) << std::endl;

    var = 3.14; // 存储double
    std::cout << "double: " << std::get<double>(var) << std::endl;

    var = std::string("hello"); // 存储string
    std::cout << "string: " << std::get<std::string>(var) << std::endl;

    std::cout << std::endl;
}

// 示例2: 访问 variant 的值
void access_variant()
{
    std::cout << "=== 访问 variant 的值 ===" << std::endl;

    std::variant<int, double, std::string> var = 42;

    // 方法1: std::get<T>() - 指定类型获取，类型不匹配时抛出异常
    try
    {
        int value = std::get<int>(var);
        std::cout << "通过类型获取: " << value << std::endl;

        // 尝试获取错误类型
        double wrong = std::get<double>(var); // 抛出 std::bad_variant_access
    }
    catch (const std::bad_variant_access &e)
    {
        std::cout << "捕获异常: " << e.what() << std::endl;
    }

    // 方法2: std::get<I>() - 通过索引获取（从0开始）
    std::cout << "通过索引获取: " << std::get<0>(var) << std::endl;

    // 方法3: std::get_if<T>() - 指针方式获取，失败返回 nullptr
    if (int *ptr = std::get_if<int>(&var))
    {
        std::cout << "通过指针获取: " << *ptr << std::endl;
    }

    if (double *ptr = std::get_if<double>(&var))
    {
        std::cout << "这个不会打印" << std::endl;
    }
    else
    {
        std::cout << "不是 double 类型" << std::endl;
    }

    std::cout << std::endl;
}

// 示例3: 类型检查
void type_inspection()
{
    std::cout << "=== 类型检查 ===" << std::endl;

    std::variant<int, double, std::string> var;

    var = 42;
    std::cout << "当前索引: " << var.index() << std::endl;

    var = 3.14;
    std::cout << "当前索引: " << var.index() << std::endl;

    var = std::string("hello");
    std::cout << "当前索引: " << var.index() << std::endl;

    std::cout << std::endl;
}

// 示例4: std::visit - 访问者模式
void visitor_pattern()
{
    std::cout << "=== 访问者模式 (std::visit) ===" << std::endl;

    // 定义一个访问者（可调用对象）
    auto visitor = [](auto &&arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            std::cout << "int: " << arg << std::endl;
        else if constexpr (std::is_same_v<T, double>)
            std::cout << "double: " << arg << std::endl;
        else if constexpr (std::is_same_v<T, std::string>)
            std::cout << "string: " << arg << std::endl;
    };

    std::variant<int, double, std::string> var;

    var = 42;
    std::visit(visitor, var);

    var = 3.14;
    std::visit(visitor, var);

    var = std::string("hello");
    std::visit(visitor, var);

    // 多个 variant 一起访问
    std::variant<int, std::string> var1 = 10;
    std::variant<int, std::string> var2 = "world";

    auto visitor2 = [](auto &&a, auto &&b)
    {
        std::cout << a << " + " << b << std::endl;
    };

    std::visit(visitor2, var1, var2);

    std::cout << std::endl;
}

// 示例5: 实际应用：配置系统
struct ConfigValue
{
    std::variant<int, double, bool, std::string> value;

    ConfigValue() : value(0) {}

    ConfigValue(int v) : value(v) {}
    ConfigValue(double v) : value(v) {}
    ConfigValue(bool v) : value(v) {}
    ConfigValue(const std::string &v) : value(v) {}
    ConfigValue(const char *v) : value(std::string(v)) {}

    // 获取值，带默认值
    template <typename T>
    T get(T default_val = T{}) const
    {
        if (const T *ptr = std::get_if<T>(&value))
        {
            return *ptr;
        }
        return default_val;
    }

    // 转换为字符串
    std::string toString() const
    {
        return std::visit([](auto &&arg) -> std::string
                          {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>)
                return arg;
            else
                return std::to_string(arg); },
                          value);
    }
};

void config_system_example()
{
    std::cout << "=== 配置系统示例 ===" << std::endl;

    std::map<std::string, ConfigValue> config;

    config["port"] = 8080;
    config["timeout"] = 3.5;
    config["debug"] = true;
    config["server_name"] = "localhost";

    std::cout << "port: " << config["port"].get<int>() << std::endl;
    std::cout << "timeout: " << config["timeout"].get<double>() << std::endl;
    std::cout << "debug: " << (config["debug"].get<bool>() ? "true" : "false") << std::endl;
    std::cout << "server_name: " << config["server_name"].get<std::string>() << std::endl;

    // 使用默认值
    std::cout << "unknown_key: " << config["unknown_key"].get<int>(-1) << std::endl;

    std::cout << std::endl;
}

// 示例6: 错误处理（Result模式）
template <typename T>
using Result = std::variant<T, std::string>;

Result<int> divide(int a, int b)
{
    if (b == 0)
    {
        return "Division by zero";
    }
    return a / b;
}

void error_handling_example()
{
    std::cout << "=== 错误处理示例 ===" << std::endl;

    auto result1 = divide(10, 2);
    std::visit([](auto &&arg)
               {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            std::cout << "结果: " << arg << std::endl;
        else if constexpr (std::is_same_v<T, std::string>)
            std::cout << "错误: " << arg << std::endl; },
               result1);

    auto result2 = divide(10, 0);
    std::visit([](auto &&arg)
               {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            std::cout << "结果: " << arg << std::endl;
        else if constexpr (std::is_same_v<T, std::string>)
            std::cout << "错误: " << arg << std::endl; },
               result2);

    std::cout << std::endl;
}

int main()
{
    basic_usage();
    access_variant();
    type_inspection();
    visitor_pattern();
    config_system_example();
    error_handling_example();

    std::cout << "std::variant 示例运行完成！" << std::endl;
    return 0;
}

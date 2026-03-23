#include <gtest/gtest.h>

#include <functional>
#include <utility>
#include <vector>

#include "types/function_view.hpp"

// 测试辅助仿函数
struct AddFunctor
{
    int operator()(int a, int b) const { return a + b; }
};

struct CounterFunctor
{
    mutable int count = 0;
    int operator()() { return ++count; }
};

// 测试默认构造函数
TEST(FunctionViewTest, DefaultConstructor)
{
    FunctionView<int(int, int)> fv;
    EXPECT_FALSE(fv);
}

// 测试 nullptr 构造函数
TEST(FunctionViewTest, NullptrConstructor)
{
    FunctionView<int(int, int)> fv = nullptr;
    EXPECT_FALSE(fv);

    FunctionView<void()> fv2(nullptr);
    EXPECT_FALSE(fv2);
}



// 测试 lambda 表达式构造
TEST(FunctionViewTest, LambdaConstructor)
{
    auto lambda = [](int a, int b)
    {
        return a * b;
    };

    FunctionView<int(int, int)> fv = lambda;
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(3, 4), 12);
}

// 测试带捕获的 lambda
TEST(FunctionViewTest, LambdaWithCapture)
{
    int multiplier = 10;
    auto lambda = [multiplier](int a, int b)
    { return (a + b) * multiplier; };
    FunctionView<int(int, int)> fv = lambda;
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(3, 4), 70);
}

// 测试无参数 lambda
TEST(FunctionViewTest, LambdaNoParameters)
{
    auto lambda = []()
    { return 42; };
    FunctionView<int()> fv = lambda;
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(), 42);
}

// 测试返回 void 的 lambda
TEST(FunctionViewTest, LambdaVoidReturn)
{
    int counter = 0;
    auto lambda = [&counter]()
    { counter++; };
    FunctionView<void()> fv = lambda;
    EXPECT_TRUE(fv);
    EXPECT_EQ(counter, 0);
    fv();
    EXPECT_EQ(counter, 1);
    fv();
    EXPECT_EQ(counter, 2);
}

// 测试仿函数构造
TEST(FunctionViewTest, FunctorConstructor)
{
    AddFunctor functor;
    FunctionView<int(int, int)> fv = functor;
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(5, 7), 12);
}

// 测试带状态的仿函数
TEST(FunctionViewTest, FunctorWithState)
{
    CounterFunctor counter;
    FunctionView<int()> fv = counter;
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(), 1);
    EXPECT_EQ(fv(), 2);
    EXPECT_EQ(fv(), 3);
}

// 测试无参数函数
TEST(FunctionViewTest, NoParameterFunction)
{
    FunctionView<int()> fv = []() { return 42; };
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(), 42);
}

// 测试返回 void 的函数
TEST(FunctionViewTest, VoidReturnFunction)
{
    int counter = 0;
    auto increment = [&counter]()
    { counter++; };

    FunctionView<void()> fv = increment;
    EXPECT_TRUE(fv);
    fv();
    EXPECT_EQ(counter, 1);
    fv();
    fv();
    EXPECT_EQ(counter, 3);
}

// 测试 bool 转换运算符 - 真值
TEST(FunctionViewTest, BoolConversionTrue)
{
    FunctionView<int(int, int)> fv = [](int a, int b) { return a + b; };
    EXPECT_TRUE(static_cast<bool>(fv));

    if (fv)
    {
        SUCCEED();
    }
    else
    {
        FAIL();
    }
}

// 测试 bool 转换运算符 - 假值
TEST(FunctionViewTest, BoolConversionFalse)
{
    FunctionView<int(int, int)> fv1;
    FunctionView<int(int, int)> fv2 = nullptr;

    EXPECT_FALSE(static_cast<bool>(fv1));
    EXPECT_FALSE(static_cast<bool>(fv2));

    if (fv1)
    {
        FAIL();
    }
    else
    {
        SUCCEED();
    }
}

// 测试多次调用同一 FunctionView
TEST(FunctionViewTest, MultipleCalls)
{
    FunctionView<int(int, int)> fv = [](int a, int b) { return a + b; };

    EXPECT_EQ(fv(1, 2), 3);
    EXPECT_EQ(fv(10, 20), 30);
    EXPECT_EQ(fv(-5, 5), 0);
    EXPECT_EQ(fv(100, 200), 300);
}

// 测试引用参数
TEST(FunctionViewTest, ReferenceParameter)
{
    auto modify = [](int &x)
    { x *= 2; };
    FunctionView<void(int &)> fv = modify;

    int value = 5;
    fv(value);
    EXPECT_EQ(value, 10);
    fv(value);
    EXPECT_EQ(value, 20);
}

// 测试 const 引用参数
TEST(FunctionViewTest, ConstReferenceParameter)
{
    auto sum = [](const int &a, const int &b)
    { return a + b; };
    FunctionView<int(const int &, const int &)> fv = sum;

    int a = 10, b = 20;
    EXPECT_EQ(fv(a, b), 30);
}

// 测试右值引用参数
TEST(FunctionViewTest, RValueReferenceParameter)
{
    auto consume = [](int &&x)
    { return x * 2; };
    FunctionView<int(int &&)> fv = consume;

    EXPECT_EQ(fv(5), 10);
    EXPECT_EQ(fv(10), 20);
}

// 测试完美转发参数
TEST(FunctionViewTest, PerfectForwarding)
{
    auto identity = [](auto &&x)
    { return std::forward<decltype(x)>(x); };
    FunctionView<int(int &&)> fv = identity;

    EXPECT_EQ(fv(42), 42);
}

// 测试复杂返回类型
TEST(FunctionViewTest, ComplexReturnType)
{
    auto get_pair = [](int a, int b)
    { return std::make_pair(a, b); };
    FunctionView<std::pair<int, int>(int, int)> fv = get_pair;

    auto result = fv(3, 5);
    EXPECT_EQ(result.first, 3);
    EXPECT_EQ(result.second, 5);
}

// 测试作为函数参数
TEST(FunctionViewTest, AsFunctionParameter)
{
    auto apply = [](FunctionView<int(int, int)> func, int a, int b)
    {
        return func(a, b);
    };

    EXPECT_EQ(apply([](int a, int b) { return a + b; }, 5, 3), 8);
    EXPECT_EQ(apply([](int a, int b) { return a * b; }, 4, 6), 24);

    auto lambda = [](int a, int b)
    { return a - b; };
    EXPECT_EQ(apply(lambda, 10, 3), 7);
}

// 测试使用 std::bind 的对象
// 测试成员函数指针
TEST(FunctionViewTest, MemberFunctionPointer)
{
    struct TestClass
    {
        int value;
        int getValue() const { return value; }
        void setValue(int v) { value = v; }
    };

    TestClass obj{42};

    // 测试 const 成员函数
    auto get_value_lambda = [&obj]()
    { return obj.getValue(); };
    FunctionView<int()> fv_get = get_value_lambda;
    EXPECT_EQ(fv_get(), 42);

    // 测试非 const 成员函数
    auto set_value_lambda = [&obj](int v)
    { obj.setValue(v); };
    FunctionView<void(int)> fv_set = set_value_lambda;
    fv_set(100);
    EXPECT_EQ(obj.value, 100);
}

// 测试临时 lambda
TEST(FunctionViewTest, TemporaryLambda)
{
    // 直接使用临时 lambda 构造 FunctionView
    FunctionView<int(int, int)> fv = [](int a, int b)
    { return a * b; };
    EXPECT_EQ(fv(3, 4), 12);
}

// 测试空 FunctionView 调用断言
TEST(FunctionViewTest, EmptyFunctionViewCallDeathTest)
{
    FunctionView<int(int, int)> fv;

    // 空的 FunctionView 调用应该触发断言
    EXPECT_DEATH(fv(1, 2), "");
}

// 测试 nullptr FunctionView 调用断言
TEST(FunctionViewTest, NullptrFunctionViewCallDeathTest)
{
    FunctionView<int(int, int)> fv = nullptr;

    // null FunctionView 调用应该触发断言
    EXPECT_DEATH(fv(1, 2), "");
}

// 测试不同签名类型的 FunctionView
TEST(FunctionViewTest, DifferentSignatures)
{
    // int(int, int)
    FunctionView<int(int, int)> fv1 = [](int a, int b) { return a + b; };
    EXPECT_EQ(fv1(3, 4), 7);

    // int()
    FunctionView<int()> fv2 = []() { return 42; };
    EXPECT_EQ(fv2(), 42);

    // void()
    FunctionView<void()> fv3 = []() {};
    EXPECT_TRUE(fv3);

    // int(int)
    FunctionView<int(int)> fv4 = [](int x)
    { return x * x; };
    EXPECT_EQ(fv4(5), 25);
}

// 测试 lambda 捕获复杂类型
TEST(FunctionViewTest, LambdaCaptureComplexType)
{
    std::vector<int> vec = {1, 2, 3};

    auto sum_vector = [vec]()
    {
        int total = 0;
        for (int v : vec)
        {
            total += v;
        }
        return total;
    };

    FunctionView<int()> fv = sum_vector;
    EXPECT_EQ(fv(), 6);
}

// 测试可变参数模板（虽然 FunctionView 不直接支持可变参数，但测试固定参数）
TEST(FunctionViewTest, MultipleParameters)
{
    auto sum_five = [](int a, int b, int c, int d, int e)
    {
        return a + b + c + d + e;
    };

    FunctionView<int(int, int, int, int, int)> fv = sum_five;
    EXPECT_EQ(fv(1, 2, 3, 4, 5), 15);
}

// 测试嵌套调用
TEST(FunctionViewTest, NestedCalls)
{
    auto add1 = [](int x)
    { return x + 1; };
    auto multiply2 = [](int x)
    { return x * 2; };

    FunctionView<int(int)> fv1 = add1;
    FunctionView<int(int)> fv2 = multiply2;

    EXPECT_EQ(fv2(fv1(5)), 12); // (5 + 1) * 2 = 12
}

// 测试返回引用
TEST(FunctionViewTest, ReturnReference)
{
    int value = 10;
    auto get_ref = [&value]() -> int &
    { return value; };

    FunctionView<int &()> fv = get_ref;
    fv() = 20;
    EXPECT_EQ(value, 20);
}

// 测试 const FunctionView
TEST(FunctionViewTest, ConstFunctionView)
{
    const FunctionView<int(int, int)> fv = [](int a, int b) { return a + b; };
    EXPECT_TRUE(fv);
    EXPECT_EQ(fv(3, 4), 7);
}

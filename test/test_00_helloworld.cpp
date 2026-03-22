#include <gtest/gtest.h>

// 简单的测试案例：验证 gtest 是否正常工作
TEST(GtestBasicTest, AssertionWorks) {
    // 测试基本的断言
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

// 测试数学运算
TEST(GtestBasicTest, MathOperations) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_NE(2 + 2, 5);
    EXPECT_GT(5, 3);
    EXPECT_LT(3, 5);
    EXPECT_GE(5, 5);
    EXPECT_LE(3, 3);
}

// 测试字符串比较
TEST(GtestBasicTest, StringComparison) {
    std::string str = "Hello World";
    EXPECT_EQ(str, "Hello World");
    EXPECT_STRCASEEQ(str.c_str(), "hello world");
}


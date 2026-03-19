#include <gtest/gtest.h>

#include "base/private/dpclass.h"

/**
 * @brief DP 指针模式测试用例
 *
 * 测试 DP（D-Pointer）模式的功能特性：
 * 1. 基本使用：构造、方法调用、获取值
 * 2. 多次调用：验证状态持久性
 * 3. 对象生命周期：析构时的资源释放
 * 4. 指针使用：堆对象创建和销毁
 * 5. 内存管理：使用 std::unique_ptr 自动管理 DPClassPrivate 对象的内存
 *
 * 注意：DPClass 禁用了拷贝和移动语义（Q_DISABLE_COPY_MOVE）
 */

// 示例 1：基本使用测试
TEST(DPClassTest, BasicUsage)
{
    DPClass obj;
    obj.publicFunction(10);
    int value = obj.publicValue();

    EXPECT_EQ(value, 42); // publicFunction 内部会调用 someInternalOperation() 设置值为 42
}

// 示例 2：多次调用测试
TEST(DPClassTest, MultipleCalls)
{
    DPClass obj;

    // 第一次调用
    obj.publicFunction(5);
    int firstValue = obj.publicValue();

    // 第二次调用
    obj.publicFunction(20);
    int secondValue = obj.publicValue();

    EXPECT_EQ(firstValue, 42); // 每次调用都会被 someInternalOperation() 设置为 42
    EXPECT_EQ(secondValue, 42);
}

// 示例 3：对象生命周期测试
TEST(DPClassTest, ObjectLifecycle)
{
    // 在作用域内创建对象
    {
        DPClass obj;
        obj.publicFunction(100);
        EXPECT_EQ(obj.publicValue(), 42);
    }

    // 对象在作用域结束时自动销毁，私有对象也被正确释放
    // 如果没有内存泄漏，测试就会通过
    SUCCEED();
}

// 示例 4：使用指针测试
TEST(DPClassTest, UsingPointers)
{
    DPClass *obj = new DPClass();
    obj->publicFunction(30);
    EXPECT_EQ(obj->publicValue(), 42);

    // 使用完成后记得释放内存
    delete obj;
    SUCCEED();
}

// 测试 setPublicValue 和 publicValue
TEST(DPClassTest, SetValue)
{
    DPClass obj;
    obj.setPublicValue(100);
    EXPECT_EQ(obj.publicValue(), 100);

    obj.setPublicValue(200);
    EXPECT_EQ(obj.publicValue(), 200);
}

// 测试默认构造后的值
TEST(DPClassTest, DefaultValue)
{
    DPClass obj;
    EXPECT_EQ(obj.publicValue(), 0);
}

// 测试多次 set 和 get
TEST(DPClassTest, MultipleSetAndGet)
{
    DPClass obj;

    for (int i = 0; i < 100; ++i)
    {
        obj.setPublicValue(i);
        EXPECT_EQ(obj.publicValue(), i);
    }
}

// 测试 publicFunction 的行为
TEST(DPClassTest, PublicFunctionBehavior)
{
    DPClass obj;

    // publicFunction 内部会调用 someInternalOperation()，将值设为 42
    obj.publicFunction(10);
    EXPECT_EQ(obj.publicValue(), 42);

    obj.publicFunction(100);
    EXPECT_EQ(obj.publicValue(), 42);
}

// 测试智能指针的内存管理（确保自动释放）
TEST(DPClassTest, SmartPointerMemoryManagement)
{
    // 创建多个对象并销毁，验证智能指针自动管理内存
    for (int i = 0; i < 100; ++i)
    {
        DPClass obj;
        obj.setPublicValue(i);
        EXPECT_EQ(obj.publicValue(), i);
    }

    // 循环结束后，所有对象的 DPClassPrivate 都已被自动释放
    // 如果没有内存泄漏，测试就会通过
    SUCCEED();
}

// 测试使用 unique_ptr 管理 DPClass（可选的高级用法）
TEST(DPClassTest, UniquePtrWrapper)
{
    auto objPtr = std::make_unique<DPClass>();
    objPtr->setPublicValue(42);
    EXPECT_EQ(objPtr->publicValue(), 42);

    // objPtr 超出作用域时，会自动释放 DPClass 及其 DPClassPrivate
    SUCCEED();
}

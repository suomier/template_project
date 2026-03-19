#include "dpclass.h"

// 私有类定义（完全隐藏于 .cpp 中）
class DPClassPrivate
{
    // 声明 q_func 和友元，便于私有类访问公共类
    DECLARE_PUBLIC(DPClass)
public:
    explicit DPClassPrivate(DPClass *parent)
        : q_ptr(parent), m_value(0)
    {
    }

    // 私有类中调用公共方法
    void someInternalOperation()
    {
        // 示例：通过 q_func 调用公共类方法
        DP_Q(DPClass);

        // 调用公共接口（需确保该方法非 const 或合理）
        q->setPublicValue(42);
    }

    int m_value;
};

// DPClass 构造函数：初始化私有指针（使用智能指针）
DPClass::DPClass()
    : d_ptr(std::make_unique<DPClassPrivate>(this))
{
}

// 析构函数：unique_ptr 自动管理内存，无需手动 delete
DPClass::~DPClass()
{
    // unique_ptr 会自动释放 d_ptr 指向的对象
}

// 公共成员函数实现
void DPClass::publicFunction(int value)
{
    DP_D(DPClass); // 获取私有对象指针 d
    d->m_value = value;

    // 调用私有类方法
    d->someInternalOperation();
}

void DPClass::setPublicValue(int val)
{
    // 非const函数
    DP_D(DPClass);
    d->m_value = val;
}

int DPClass::publicValue() const
{
    // const函数
    DP_CD(DPClass);
    return d->m_value;
}

void DPClass::setPrivateValue(int val)
{
    DP_D(DPClass);
    d->m_value = val;
}

int DPClass::privateValue() const
{
    DP_CD(DPClass);
    return d->m_value;
}
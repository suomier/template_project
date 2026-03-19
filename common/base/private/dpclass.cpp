#include "dpclass.h"

// 私有类定义（完全隐藏于 .cpp 中）
class DPClassPrivate
{
    // 声明 q_func 和友元，便于私有类访问公共类
    Q_DECLARE_PUBLIC(DPClass)
public:
    explicit DPClassPrivate(DPClass *parent)
        : q_ptr(parent), m_value(0)
    {
    }

    void someInternalOperation()
    {
        // 示例：通过 q_func 调用公共类方法
        Q_Q(DPClass);

        // 调用公共接口（需确保该方法非 const 或合理）
        q->setPublicValue(42);
    }

    int m_value;
};

// DPClass 构造函数：初始化私有指针
DPClass::DPClass()
    : d_ptr(new DPClassPrivate(this))
{

}

// 析构函数：释放私有对象
DPClass::~DPClass()
{
    delete d_ptr;
}

// 公共成员函数实现
void DPClass::publicFunction(int value)
{
    Q_D(DPClass); // 获取私有对象指针 d
    d->m_value = value;

    // 调用私有类方法
    d->someInternalOperation();
}

void DPClass::setPublicValue(int val)
{
    // 非const函数
    Q_D(DPClass);
    d->m_value = val;
}

int DPClass::publicValue() const
{
    // const函数
    Q_CD(DPClass);
    return d->m_value;
}

void DPClass::setPrivateValue(int val)
{
    Q_D(DPClass);
    d->m_value = val;
}

int DPClass::privateValue() const
{
    Q_CD(DPClass);
    return d->m_value;
}
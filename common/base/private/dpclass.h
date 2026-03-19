#ifndef DPCLASS_H
#define DPCLASS_H

#include "base/common_def.h"

// DP 指针例子

class DPClassPrivate;

class DPClass
{
    DECLARE_PRIVATE(DPClass)
    DISABLE_COPY_MOVE(DPClass)
public:
    DPClass();
    ~DPClass();

    // 公共接口方法示例
    void publicFunction(int value);

    void setPublicValue(int val);
    int publicValue() const;

private:
    void setPrivateValue(int val);
    int privateValue() const;
};

#endif // DPCLASS_H

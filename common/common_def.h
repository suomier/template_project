#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <cstdlib>

// 模块名称
enum E_Module_Type
{
    E_Module_None = 0,
    E_Module_Log,
    E_Module_Max,
};

static const char *getModuleType(E_Module_Type type)
{
    switch (type)
    {
    case E_Module_None:
        return "";
    case E_Module_Log:
        return "log";
    default:
        return "";
    };
}

#endif // COMMON_DEF_H

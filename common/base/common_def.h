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

const char *getModuleType(E_Module_Type type);

#endif // COMMON_DEF_H

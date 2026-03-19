#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

// 模块名称
enum E_Module_Type
{
    E_Module_None = 0,
    E_Module_Log,
    E_Module_Max,
};

const char *getModuleType(E_Module_Type type);

#endif // COMMON_TYPES_H

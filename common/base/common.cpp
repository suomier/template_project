#include "common_def.h"
#include "common_types.h"

const char *getModuleType(E_Module_Type type)
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
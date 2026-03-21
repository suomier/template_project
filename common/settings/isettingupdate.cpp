#include "isettingupdate.h"

ISettingUpdate::ISettingUpdate(E_Module_Type eType)
    : eType_(eType)
{
}

E_Module_Type ISettingUpdate::getModuleType() const
{
    return eType_;
}
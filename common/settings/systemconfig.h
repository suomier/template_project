#ifndef SYSTEMCONFIG_H
#define SYSTEMCONFIG_H

#include "base/common_def.h"
#include "base/common_types.h"

#include "settings/isettingupdate.h"

struct ConfigValue;
class SystemConfigPrivate;

class SystemConfig final
{
private:
    DECLARE_PRIVATE(SystemConfig)
    DISABLE_COPY_MOVE(SystemConfig)

    explicit SystemConfig();
    ~SystemConfig();

public:
    static SystemConfig *Instance();

    void setUpdateReceive(ISettingUpdate *receiver);

    const ConfigValue &getVal(E_Module_Type eType, const char *key);
};

#endif // SYSTEMCONFIG_H

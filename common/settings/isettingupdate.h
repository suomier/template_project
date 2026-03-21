#ifndef ISETTINGUPDATE_H
#define ISETTINGUPDATE_H

#include "base/common_def.h"
#include "base/common_types.h"

struct ConfigValue
{
    ConfigValue() : value(0) {}
    ConfigValue(int v) : value(v) {}
    ConfigValue(uint64_t v) : value(v) {}
    ConfigValue(bool v) : value(v) {}
    ConfigValue(const std::string &v) : value(v) {}
    ConfigValue(const char *v) : value(std::string(v)) {}

    // 获取值，带默认值
    template <typename T>
    T get(T default_val = T{}) const
    {
        if (const T *ptr = std::get_if<T>(&value))
        {
            return *ptr;
        }
        return default_val;
    }

    // 转换为字符串
    std::string toString() const;

    std::variant<int, uint64_t, bool, std::string> value;
};

class ISettingUpdate
{
public:
    ISettingUpdate() = delete;
    ISettingUpdate(E_Module_Type eType);

    ISettingUpdate(const ISettingUpdate &) = delete;
    ISettingUpdate &operator=(const ISettingUpdate &) = delete;

    ISettingUpdate(ISettingUpdate &&) = delete;
    ISettingUpdate &operator=(ISettingUpdate &&) = delete;

    virtual ~ISettingUpdate() = default;

    E_Module_Type getModuleType() const;

    virtual void configUpdate(E_Module_Type eType, const char *key, const ConfigValue &val) = 0;

protected:
    E_Module_Type eType_ = E_Module_None;
};

#endif // ISETTINGUPDATE_H

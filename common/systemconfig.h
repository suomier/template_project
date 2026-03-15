#ifndef SYSTEMCONFIG_H
#define SYSTEMCONFIG_H

#include <cstdint>
#include <cstring>
#include <string>
#include <variant>

#include "common_def.h"

struct ConfigValue
{
    std::variant<int, uint64_t, bool, std::string> value;

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
};

/**
 * 设置变化
 */
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

class SystemConfigPrivate;

class SystemConfig
{
public:
    static SystemConfig *Instance();

    void setUpdateReceive(ISettingUpdate *receiver);

    const ConfigValue &getVal(E_Module_Type eType, const char *key);

private:
    explicit SystemConfig();
    ~SystemConfig();
    SystemConfig(const SystemConfig &) = delete;
    SystemConfig &operator=(const SystemConfig &) = delete;

    SystemConfigPrivate *d_;
};

#endif // SYSTEMCONFIG_H

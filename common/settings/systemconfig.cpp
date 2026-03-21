#include "systemconfig.h"

#include <filesystem>
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <array>

std::string ConfigValue::toString() const
{
    return std::visit([](auto &&arg) -> std::string
                      {
                          using T = std::decay_t<decltype(arg)>;

                          if constexpr (std::is_same_v<T, std::string>)
                          {
                              return arg;
                          }
                          else
                          {
                              return std::to_string(arg);
                          } },
                      value);
}

class SystemConfigPrivate final
{
    DECLARE_PUBLIC(SystemConfig)
public:
    SystemConfigPrivate(SystemConfig *parent)
        : q_ptr(parent)
    {
        // 1. 获取程序路径
        std::string exePath;

#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        exePath = buffer;
#else
        char buffer[PATH_MAX];
        if (readlink("/proc/self/exe", buffer, sizeof(buffer)) != -1)
        {
            exePath = buffer;
        }
#endif

        // 获取程序所在目录
        std::filesystem::path exePathObj(exePath);
        std::string exeDir = exePathObj.parent_path().string();

        // 构建配置文件路径
        std::string configPath = exeDir + "/config/base_config.yaml";

        // 2. 使用yaml-cpp读取配置
        YAML::Node yaml_config;

        try
        {
            yaml_config = YAML::LoadFile(configPath);
        }
        catch (const YAML::Exception &e)
        {
            fprintf(stderr, "load %s config file failed\n", configPath.c_str());
            return;
        }

        // 3. 解析配置
        for (int i = 0; i < E_Module_Max; ++i)
        {
            E_Module_Type moduleType = static_cast<E_Module_Type>(i);
            const char *moduleName = getModuleType(moduleType);

            if (!moduleName || strlen(moduleName) == 0)
            {
                continue;
            }

            // 获取该模块的配置节点
            if (yaml_config[moduleName])
            {
                auto moduleNode = yaml_config[moduleName];

                // 遍历该模块的所有配置项
                for (auto it = moduleNode.begin(); it != moduleNode.end(); ++it)
                {
                    std::string key = it->first.as<std::string>();
                    auto value = it->second;

                    // 根据值的类型创建ConfigValue
                    if (value.IsScalar())
                    {
                        do
                        {
                            try
                            {
                                // 尝试解析为bool
                                config_[moduleType][key] = value.as<bool>();
                                break;
                            }
                            catch (...)
                            {
                            }

                            try
                            {
                                // 尝试解析为int
                                config_[moduleType][key] = value.as<int>();
                                break;
                            }
                            catch (...)
                            {
                            }

                            try
                            {
                                // 尝试解析为uint64_t
                                config_[moduleType][key] = value.as<uint64_t>();
                                break;
                            }
                            catch (...)
                            {
                            }

                            // 最后解析为string
                            config_[moduleType][key] = value.as<std::string>();
                        } while (false);
                    }
                }
            }
        }
    }

    ~SystemConfigPrivate() = default;

    void setUpdateReceive(ISettingUpdate *receiver)
    {
        m_updateReceiver[receiver->getModuleType()] = receiver;
    }

    const ConfigValue &getVal(E_Module_Type eType, const char *key)
    {
        auto &configMap = config_[eType];
        auto it = configMap.find(key);
        if (it != configMap.end())
        {
            return it->second;
        }
        return default_;
    }

    static inline ConfigValue default_{0};

private:
    std::array<ISettingUpdate *, E_Module_Max> m_updateReceiver;
    std::array<std::map<std::string, ConfigValue>, E_Module_Max> config_;
};

SystemConfig::SystemConfig()
    : d_ptr(std::make_unique<SystemConfigPrivate>(this))
{
}

SystemConfig::~SystemConfig() = default;

SystemConfig *SystemConfig::Instance()
{
    static SystemConfig ins;
    return &ins;
}

void SystemConfig::setUpdateReceive(ISettingUpdate *receiver)
{
    DP_D(SystemConfig);
    d->setUpdateReceive(receiver);
}

const ConfigValue &SystemConfig::getVal(E_Module_Type eType, const char *key)
{
    DP_D(SystemConfig);
    return d->getVal(eType, key);
}

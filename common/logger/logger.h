#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "settings/systemconfig.h"

// 使用 spdlog 封装以下4种日志：
// 1.每日循环日志，无论任何级别都记录，记录在目录：logs/daily，只记录最近一天的，超出时间则自动删除
// 2.通过配置开启，develop日志，开启时输出到终端，自动输出到 logs/dev；存在 log_dev 时, release模式下无法编译；
// 3.error级别以上的日志, 立即刷新, 自动输出到 logs/error, 永远不删除;
// 4.log_info级别日志, 可以配置级别、运行时支持动态配置；配置大小，自动压缩;日志位于: logs/trace

class Logger
{
private:
    Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    virtual ~Logger();

    // 格式化日志消息 [module] [file:line:func] msg
    template <typename... Args>
    std::string format_with_loc(const char *module, const char *filepath, int line, const char *func, const char *msg, Args... args)
    {
        // 提取文件名（跨平台）
        const char *filename = filepath;
        const char *last_sep = nullptr;
#ifdef _WIN32
        last_sep = strrchr(filepath, '\\');
#else
        last_sep = strrchr(filepath, '/');
#endif
        if (last_sep)
        {
            filename = last_sep + 1;
        }

        std::string prefix = "[";
        if (module && strlen(module) > 0)
        {
            prefix += module;
            prefix += "] ";
        }
        else
        {
            prefix += "] ";
        }
        prefix += "[";
        prefix += filename;
        prefix += ":";
        prefix += std::to_string(line);
        prefix += ":";
        prefix += func;
        prefix += "] ";
        prefix += msg;
        return fmt::format(prefix.c_str(), args...);
    }

public:
    static Logger *Instance();

    // 初始化所有日志
    void Init();
    void UnInit();

    // 日志
    template <typename... Args>
    void develop(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (develop_logger_)
        {
            develop_logger_->info(formatted);
        }
    }

    // 日志记录接口（供宏调用）
    template <typename... Args>
    void trace(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->trace(formatted);
        }
    }

    template <typename... Args>
    void debug(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->debug(formatted);
        }
    }

    template <typename... Args>
    void info(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->info(formatted);
        }
    }

    template <typename... Args>
    void warn(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->warn(formatted);
        }
    }

    template <typename... Args>
    void error(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->error(formatted);
        }
    }

    template <typename... Args>
    void critical(const char *module, const char *filename, int line, const char *func, const char *msg, Args... args)
    {
        auto formatted = format_with_loc(module, filename, line, func, msg, args...);
        if (logger_)
        {
            logger_->critical(formatted);
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> develop_logger_;
};

#ifndef ModuleName
#define ModuleName "default model"
#endif // ModuleName

#define log_dev(msg, ...) Logger::Instance()->develop(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

#define log_trace(msg, ...) Logger::Instance()->trace(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define log_debug(msg, ...) Logger::Instance()->debug(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

#define log_info(msg, ...) Logger::Instance()->info(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define log_warn(msg, ...) Logger::Instance()->warn(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define log_error(msg, ...) Logger::Instance()->error(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define log_critical(msg, ...) Logger::Instance()->critical(ModuleName, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

#endif // LOGGER_H

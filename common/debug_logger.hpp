#ifndef DEBUG_LOGGER_HPP
#define DEBUG_LOGGER_HPP

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

// 日志名称
#define log_name "../logs/debug_log.log"

class DebugLogger
{
public:
    static DebugLogger &GetInstance()
    {
        static DebugLogger m_instance;
        return m_instance;
    }

    std::shared_ptr<spdlog::logger> GetLogger()
    {
        return nml_logger;
    }

private:
    DebugLogger()
    {
        set_debug_logger();

        // 当遇到 err 或更严重的错误时立刻持久化日志
        nml_logger->flush_on(spdlog::level::err);
        // 设置 3秒刷新一次
        spdlog::flush_every(std::chrono::seconds(3));
    }

    void set_debug_logger()
    {
        // 设置为异步日志
        std::vector<spdlog::sink_ptr> log_sink_list;

        // 终端日志
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");

        // 文件日志
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_name);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");

        console_sink->set_level(spdlog::level::trace);
        file_sink->set_level(spdlog::level::debug);

        log_sink_list.push_back(console_sink);
        // log_sink_list.push_back(file_sink);

        nml_logger = std::make_shared<spdlog::logger>("both", begin(log_sink_list), end(log_sink_list));
        // 注册
        spdlog::register_logger(nml_logger);

        // 设置日志记录级别
        nml_logger->set_level(spdlog::level::trace);
    }

    ~DebugLogger()
    {
        spdlog::drop_all();
    }

    DebugLogger(const DebugLogger &) = delete;

    DebugLogger &operator=(const DebugLogger &) = delete;

private:
    std::shared_ptr<spdlog::logger> nml_logger;
};

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif

// 日志位置前缀
#define location_prefix(msg) std::string().append(" [").append(__FILENAME__).append(":").append(std::to_string(__LINE__)).append(":").append(__func__).append("] ").append(msg).c_str()

#undef log_trace
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef log_critical

#define log_trace(msg, ...) DebugLogger::GetInstance().GetLogger()->trace(location_prefix(msg), ##__VA_ARGS__)
#define log_debug(msg, ...) DebugLogger::GetInstance().GetLogger()->debug(location_prefix(msg), ##__VA_ARGS__)
#define log_info(msg, ...) DebugLogger::GetInstance().GetLogger()->info(location_prefix(msg), ##__VA_ARGS__)
#define log_warn(msg, ...) DebugLogger::GetInstance().GetLogger()->warn(location_prefix(msg), ##__VA_ARGS__)
#define log_error(msg, ...) DebugLogger::GetInstance().GetLogger()->error(location_prefix(msg), ##__VA_ARGS__)
#define log_critical(msg, ...) DebugLogger::GetInstance().GetLogger()->critical(location_prefix(msg), ##__VA_ARGS__)

#endif // DEBUG_LOGGER_HPP

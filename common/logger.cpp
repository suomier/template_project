#include "logger.h"

#include <filesystem>

Logger::Logger()
{
}

Logger *Logger::Instance()
{
    static Logger instance;
    return &instance;
}

Logger::~Logger()
{
    spdlog::drop_all();
}

void Logger::Init()
{
    // 从配置文件获取日志配置
    auto log_dir = SystemConfig::Instance()->getVal(E_Module_Log, "log_dir").get<std::string>("./logs");

    // 创建日志目录
    try
    {
        std::filesystem::create_directories(log_dir + "/daily");
        std::filesystem::create_directories(log_dir + "/error");
        std::filesystem::create_directories(log_dir + "/trace");
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "create log directories failed: %s\n", e.what());
    }

    // 设置为异步日志
    std::vector<spdlog::sink_ptr> log_sink_list;

    if (SystemConfig::Instance()->getVal(E_Module_Log, "log_console").get<bool>(false))
    {
        // 终端日志
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");
        console_sink->set_level(spdlog::level::info);
        log_sink_list.push_back(console_sink);
    }

    // 开发日志
    if (SystemConfig::Instance()->getVal(E_Module_Log, "log_develop").get<bool>(false))
    {
        // 开发日志
        auto develop_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // develop 关键字为蓝色
        develop_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [\33[1;34mdevelop \33[0m] %v");;
        develop_sink->set_level(spdlog::level::info);

        develop_logger_ = std::make_shared<spdlog::logger>("develop_sink", develop_sink);
        spdlog::register_logger(develop_logger_);
        develop_logger_->set_level(spdlog::level::info);
    }

    // 每日循环日志
    if (SystemConfig::Instance()->getVal(E_Module_Log, "log_daily").get<bool>(false))
    {
        std::string daily_log_path = log_dir + "/daily/daily.log";

        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(daily_log_path, 0, 0);
        daily_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");
        daily_sink->set_level(spdlog::level::trace);
        log_sink_list.push_back(daily_sink);
    }

    // 错误日志
    if (SystemConfig::Instance()->getVal(E_Module_Log, "log_error").get<bool>(false))
    {
        std::string error_log_path = log_dir + "/error/error.log";

        auto error_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(error_log_path, true);
        error_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");
        error_sink->set_level(spdlog::level::err);
        log_sink_list.push_back(error_sink);
    }

    // 4. 追踪日志：可配置级别，运行时支持动态配置，配置大小，自动压缩，位于 logs/trace
    bool log_zip = SystemConfig::Instance()->getVal(E_Module_Log, "log_zip").get<bool>(true);
    int log_size = SystemConfig::Instance()->getVal(E_Module_Log, "log_size").get<int>(250); // 单位: MB
    int log_count = SystemConfig::Instance()->getVal(E_Module_Log, "log_count").get<int>(10);

    std::string trace_log_path = log_dir + "/trace/trace.log";
    auto trace_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(trace_log_path, log_size * 1024 * 1024, log_count, log_zip);
    trace_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%f %z] [%^%=8l%$] %v");
    log_sink_list.push_back(trace_sink);

    logger_ = std::make_shared<spdlog::logger>("both", begin(log_sink_list), end(log_sink_list));
    // 当遇到 err 或更严重的错误时立刻持久化日志
    logger_->flush_on(spdlog::level::err);

    // 注册
    spdlog::register_logger(logger_);

    // 设置 3秒刷新一次
    spdlog::flush_every(std::chrono::seconds(3));

    // 设置日志记录级别
    logger_->set_level(spdlog::level::trace);
}

void Logger::UnInit()
{
    spdlog::drop_all();
}
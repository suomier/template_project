#include <iostream>

#include "logger/logger.h"

int main(int, char *[])
{
    Logger::Instance()->Init();

    // 测试 trace 级别
    log_trace("This is a trace message");
    log_debug("This is a debug message");
    log_info("This is an info message");
    log_warn("This is a warning message");
    log_error("This is an error message");
    log_critical("This is a critical message");

    // 测试带参数的日志
    log_info("User {} logged in from IP {}", "admin", "192.168.1.1");
    log_error("Failed to connect to database: {}", "connection timeout");
    log_warn("Memory usage is high: {}%", 85);

    log_dev("This is a dev message");
    log_dev("Debug info: value = {}", 42);

    // for (int i = 0; i < INT_MAX; ++i)
    // {
    //     log_info("This is an info message");
    // }

    Logger::Instance()->UnInit();
    return 0;
}

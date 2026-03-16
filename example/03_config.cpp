#include <iostream>

#include "systemconfig.h"

int main()
{
    std::cout << "log_dir     : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_dir").get<std::string>() << std::endl;
    std::cout << "log_dev     : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_dev").get<bool>() << std::endl;
    std::cout << "log_size    : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_size").get<int>() << std::endl;
    std::cout << "log_count   : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_count").get<int>() << std::endl;
    std::cout << "log_zip     : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_zip").get<bool>() << std::endl;
    std::cout << "trace_level : " << SystemConfig::Instance()->getVal(E_Module_Log, "trace_level").get<std::string>() << std::endl;
    std::cout << "log_daily : " << SystemConfig::Instance()->getVal(E_Module_Log, "log_daily").get<bool>() << std::endl;
    return 0;
}

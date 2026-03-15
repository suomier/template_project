#include <yaml-cpp/yaml.h>

#include <iostream>

struct GeneralConf
{
    std::string log_dir;
    std::string log_name;
    std::string log_level;
    bool log_to_stderr;
    int ice_min_port = 0;
    int ice_max_port = 0;
};

int load_general_conf(const char *filename, GeneralConf *conf);

int main()
{
    GeneralConf conf;

    load_general_conf("../../conf/general.yaml", &conf);
    std::cout << "log_dir: " << conf.log_dir << std::endl;
    std::cout << "log_name: " << conf.log_name << std::endl;
    std::cout << "log_level: " << conf.log_level << std::endl;

    std::cout << "log_to_stderr: " << (conf.log_to_stderr ? "true" : "false") << std::endl;
    std::cout << "ice_min_port: " << conf.ice_min_port << std::endl;
    std::cout << "ice_max_port: " << conf.ice_max_port << std::endl;

    return 0;
}

int load_general_conf(const char *filename, GeneralConf *conf)
{
    if (!filename || !conf)
    {
        fprintf(stderr, "filename or conf is nullptr\n");
        return -1;
    }

    conf->log_dir = "./log";
    conf->log_name = "undefined";
    conf->log_level = "info";
    conf->log_to_stderr = false;

    YAML::Node config = YAML::LoadFile(filename);

    try
    {
        conf->log_dir = config["log"]["log_dir"].as<std::string>();
        conf->log_name = config["log"]["log_name"].as<std::string>();
        conf->log_level = config["log"]["log_level"].as<std::string>();
        conf->log_to_stderr = config["log"]["log_to_stderr"].as<bool>();
        conf->ice_min_port = config["ice"]["min_port"].as<int>();
        conf->ice_max_port = config["ice"]["max_port"].as<int>();
    }
    catch (YAML::Exception e)
    {
        fprintf(stderr, "catch a YAML::Exception, line: %d, column: %d, error:%s\n",
                e.mark.line + 1, e.mark.column + 1, e.msg.c_str());
        return -1;
    }

    return 0;
}
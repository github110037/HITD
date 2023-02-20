#include "easylogging++.h"
#include "common.hpp"
#include <fmt/core.h>

char* log_file_name;
extern uint32_t log_pc;
extern uint64_t ticks;
static std::string now_pc(const el::LogMessage* msg){
    return fmt::format(HEX_WORD, log_pc);
}
static std::string now_ticks(const el::LogMessage* msg){
    return std::to_string(ticks);
}
static bool is_first = true;
inline static void first_init(){
    el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%pc", now_pc));
    el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%ticks", now_ticks));
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    is_first = false;
}

el::Logger* logger_init(std::string name){
    if (is_first) first_init();

    el::Configurations log_conf;
    log_conf.setToDefault();

    log_conf.setGlobally(el::ConfigurationType::Format, "[" + name + "][%ticks][%pc][%levshort]:%msg");
    log_conf.setGlobally(el::ConfigurationType::Filename, log_file_name);
    log_conf.setGlobally(el::ConfigurationType::ToFile, "true");
    log_conf.set(el::Level::Trace,   el::ConfigurationType::ToStandardOutput, "false");
    log_conf.set(el::Level::Info,    el::ConfigurationType::ToStandardOutput, "true");
    log_conf.set(el::Level::Error,   el::ConfigurationType::ToStandardOutput, "true");
    log_conf.set(el::Level::Warning, el::ConfigurationType::ToStandardOutput, "true");

    el::Logger* logger = el::Loggers::getLogger(name);
    logger->configure(log_conf);
    LOG(INFO) << "Init logger with name:" << name;
    return logger;
}
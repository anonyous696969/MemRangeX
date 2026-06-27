#ifndef MEMRANGEX_LOGGER_H
#define MEMRANGEX_LOGGER_H

#include "common_types.h"
#include <fstream>
#include <mutex>
#include <iostream>

enum LogLevel
{
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
};

class Logger
{
public:
    static Logger& GetInstance();
    void Init(const std::string& log_path, int level);
    void WriteLog(int level, const std::string& msg);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file_;
    int log_level_ = LOG_INFO;
    std::mutex log_mtx_;
};

#define LOG_DEBUG_STR(msg) Logger::GetInstance().WriteLog(LOG_DEBUG, msg)
#define LOG_INFO_STR(msg)  Logger::GetInstance().WriteLog(LOG_INFO, msg)
#define LOG_WARN_STR(msg)  Logger::GetInstance().WriteLog(LOG_WARN, msg)
#define LOG_ERROR_STR(msg) Logger::GetInstance().WriteLog(LOG_ERROR, msg)

#endif
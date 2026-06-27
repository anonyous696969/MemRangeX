#include "logger.h"
#include "utils.h"
#include <filesystem>

Logger& Logger::GetInstance()
{
    static Logger ins;
    return ins;
}

void Logger::Init(const std::string& log_path, int level)
{
    std::lock_guard<std::mutex> lk(log_mtx_);
    log_level_ = level;
    if (!log_path.empty())
    {
        std::filesystem::create_directories(log_path);
        log_file_.open(log_path + "memrangex_runtime.log", std::ios::app | std::ios::out);
    }
    if (!log_file_.is_open())
    {
        std::cerr << "[Logger Error] Open log file failed: " << log_path << std::endl;
    }
}

void Logger::WriteLog(int level, const std::string& msg)
{
    if (level < log_level_)
    {
        return;
    }

    std::lock_guard<std::mutex> lk(log_mtx_);
    TimeNs now = Utils::GetCurrentNanosecond();
    std::string level_tag;
    switch (level)
    {
        case LOG_DEBUG: level_tag = "[DEBUG]"; break;
        case LOG_INFO: level_tag = "[INFO]"; break;
        case LOG_WARN: level_tag = "[WARN]"; break;
        case LOG_ERROR: level_tag = "[ERROR]"; break;
        default: level_tag = "[UNKNOWN]"; break;
    }

    if (log_file_.is_open())
    {
        log_file_ << now << " " << level_tag << " " << msg << std::endl;
        log_file_.flush();
    }
    else
    {
        std::cerr << now << " " << level_tag << " " << msg << std::endl;
    }
}

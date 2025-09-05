/**
 * @file logger.hpp
 * @brief High-performance binary logging system
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>

namespace hfx::log {

enum class LogLevel : int {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    Logger() = default;
    ~Logger() = default;

    void info(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);
    void warn(const std::string& message);
    void critical(const std::string& message);

    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        std::stringstream ss;
        ss << format;
        ((ss << " " << args), ...);
        info(ss.str());
    }

    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        std::stringstream ss;
        ss << format;
        ((ss << " " << args), ...);
        error(ss.str());
    }

    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        std::stringstream ss;
        ss << format;
        ((ss << " " << args), ...);
        debug(ss.str());
    }

    void set_log_level(LogLevel level) { log_level_ = level; }
    LogLevel get_log_level() const { return log_level_; }

    // Singleton pattern for global access
    static Logger& instance() {
        static Logger instance_;
        return instance_;
    }

private:
    std::atomic<LogLevel> log_level_{LogLevel::INFO};
    std::mutex log_mutex_;
    
    void log_with_level(LogLevel level, const std::string& level_str, const std::string& message);
};

// Global logger macros for convenience
#define HFX_LOG_DEBUG(msg) hfx::log::Logger::instance().debug(msg)
#define HFX_LOG_INFO(msg) hfx::log::Logger::instance().info(msg)  
#define HFX_LOG_WARN(msg) hfx::log::Logger::instance().warn(msg)
#define HFX_LOG_ERROR(msg) hfx::log::Logger::instance().error(msg)
#define HFX_LOG_CRITICAL(msg) hfx::log::Logger::instance().critical(msg)

} // namespace hfx::log

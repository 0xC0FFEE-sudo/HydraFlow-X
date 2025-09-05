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
    
    // Make log_with_level public for LogStream access
    void log_with_level(LogLevel level, const std::string& level_str, const std::string& message);

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
};

// Stream-based logging helper
class LogStream {
public:
    LogStream(Logger& logger, LogLevel level) : logger_(logger), level_(level) {}
    
    ~LogStream() {
        logger_.log_with_level(level_, get_level_string(level_), stream_.str());
    }
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }
    
    LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        stream_ << manip;
        return *this;
    }

private:
    Logger& logger_;
    LogLevel level_;
    std::stringstream stream_;
    
    const char* get_level_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

// Stream-capable logger macros for convenience
#define HFX_LOG_DEBUG(...) hfx::log::Logger::instance().debug(__VA_ARGS__)
#define HFX_LOG_INFO(...) hfx::log::Logger::instance().info(__VA_ARGS__)  
#define HFX_LOG_WARN(...) hfx::log::Logger::instance().warn(__VA_ARGS__)
#define HFX_LOG_ERROR(...) hfx::log::Logger::instance().error(__VA_ARGS__)
#define HFX_LOG_CRITICAL(...) hfx::log::Logger::instance().critical(__VA_ARGS__)

// Stream-based macros for complex formatting
#define HFX_LOG_STREAM(level) hfx::log::LogStream(hfx::log::Logger::instance(), level)
#define HFX_LOG_DEBUG_STREAM() HFX_LOG_STREAM(hfx::log::LogLevel::DEBUG)
#define HFX_LOG_INFO_STREAM() HFX_LOG_STREAM(hfx::log::LogLevel::INFO)
#define HFX_LOG_WARN_STREAM() HFX_LOG_STREAM(hfx::log::LogLevel::WARN)
#define HFX_LOG_ERROR_STREAM() HFX_LOG_STREAM(hfx::log::LogLevel::ERROR)
#define HFX_LOG_CRITICAL_STREAM() HFX_LOG_STREAM(hfx::log::LogLevel::CRITICAL)

} // namespace hfx::log

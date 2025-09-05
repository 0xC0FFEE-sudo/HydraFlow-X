/**
 * @file logger.cpp
 * @brief Logger implementation
 */

#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace hfx::log {

void Logger::log_with_level(LogLevel level, const std::string& level_str, const std::string& message) {
    if (level < log_level_.load()) {
        return; // Skip logging if below threshold
    }
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    auto& output_stream = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
    output_stream << "[" << ss.str() << "] [" << level_str << "] " << message << std::endl;
}

void Logger::debug(const std::string& message) {
    log_with_level(LogLevel::DEBUG, "DEBUG", message);
}

void Logger::info(const std::string& message) {
    log_with_level(LogLevel::INFO, "INFO", message);
}

void Logger::warn(const std::string& message) {
    log_with_level(LogLevel::WARN, "WARN", message);
}

void Logger::error(const std::string& message) {
    log_with_level(LogLevel::ERROR, "ERROR", message);
}

void Logger::critical(const std::string& message) {
    log_with_level(LogLevel::CRITICAL, "CRITICAL", message);
}

} // namespace hfx::log

/**
 * @file logger.hpp
 * @brief High-performance binary logging system
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <memory>

namespace hfx::log {

class Logger {
public:
    Logger() = default;
    ~Logger() = default;

    void info(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);

    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        info(format); // Simplified for now
    }

    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        error(format); // Simplified for now
    }
};

} // namespace hfx::log

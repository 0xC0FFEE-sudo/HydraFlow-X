/**
 * @file simple_logger.hpp
 * @brief Simple fallback logger for build compatibility
 */

#pragma once

#include <iostream>
#include <sstream>

// Disabled logging macros for build compatibility
#define HFX_LOG_DEBUG(msg) do {} while(0)
#define HFX_LOG_INFO(msg) do {} while(0)  
#define HFX_LOG_WARN(msg) do {} while(0)
#define HFX_LOG_ERROR(msg) do {} while(0)
#define HFX_LOG_CRITICAL(msg) do {} while(0)

// Disabled stream versions
#define HFX_LOG_DEBUG_STREAM() if(false) std::cout
#define HFX_LOG_INFO_STREAM() if(false) std::cout
#define HFX_LOG_WARN_STREAM() if(false) std::cout
#define HFX_LOG_ERROR_STREAM() if(false) std::cout
#define HFX_LOG_CRITICAL_STREAM() if(false) std::cout

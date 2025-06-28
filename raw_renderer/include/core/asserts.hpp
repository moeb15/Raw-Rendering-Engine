#pragma once

#include "core/defines.hpp"
#include "core/logger.hpp"

#define RAW_ASSERT(expr) if(!(expr)) { RAW_FATAL(RAW_FILELINE("FALSE\n")); RAW_DEBUG_BREAK }
#if defined(RAW_PLATFORM_WINDOWS)
    #define RAW_ASSERT_MSG(expr, msg, ...) if(!(expr)) { RAW_FATAL(RAW_FILELINE(RAW_CONCAT(msg, "\n")), __VA_ARGS__); RAW_DEBUG_BREAK }
#else
    #define RAW_ASSERT_MSG(expr, msg, ...) if(!(expr)) { RAW_FATAL(RAW_FILELINE(RAW_CONCAT(msg, "\n")), ## __VA_ARGS__); RAW_DEBUG_BREAK }
#endif

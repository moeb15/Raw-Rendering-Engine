#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1 
#define LOG_TRACE_ENABLED 1

#ifdef RAW_RELEASE
    #define LOG_DEBUG_ENABLED 0
    #define LOG_TRACE_ENABLED 0
#endif

enum ELogLevel : uint8_t
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
};

namespace Raw
{
    class Logger : public IService
    {
    public:
        RAW_DECLARE_SERVICE(Logger);
        bool InititalizeLogger();
        virtual void Shutdown() override;

        void PrintMessage(ELogLevel level, cstring msg, ...);
        
        static constexpr cstring k_ServiceName = "raw_log_service";
    };
}

#if defined(_WIN32)
    #define RAW_FATAL(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_FATAL, msg, __VA_ARGS__);
    #define RAW_ERROR(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_ERROR, msg, __VA_ARGS__);

    #if LOG_WARN_ENABLED
        #define RAW_WARN(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_WARN, msg, __VA_ARGS__);
    #else
        #define RAW_WARN(msg, ...)
    #endif
    #if LOG_INFO_ENABLED
        #define RAW_INFO(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_INFO, msg, __VA_ARGS__);
    #else
        #define RAW_INFO(msg, ...)
    #endif

    #if LOG_DEBUG_ENABLED
        #define RAW_DEBUG(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_DEBUG, msg, __VA_ARGS__);
    #else
        #define RAW_DEBUG(msg, ...)
    #endif

    #if LOG_TRACE_ENABLED
        #define RAW_TRACE(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_TRACE, msg, __VA_ARGS__);
    #else
        #define RAW_TRACE(msg, ...)
    #endif
#else
    #define RAW_FATAL(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_FATAL, msg, ## __VA_ARGS__);
    #define RAW_ERROR(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_ERROR, msg, ## __VA_ARGS__);

    #if LOG_WARN_ENABLED
        #define RAW_WARN(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_WARN, msg, ## __VA_ARGS__);
    #else
        #define RAW_WARN(msg, ...)
    #endif
    #if LOG_INFO_ENABLED
        #define RAW_INFO(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_INFO, msg, ## __VA_ARGS__);
    #else
        #define RAW_INFO(msg, ...)
    #endif

    #if LOG_DEBUG_ENABLED
        #define RAW_DEBUG(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_DEBUG, msg, ## __VA_ARGS__);
    #else
        #define RAW_DEBUG(msg, ...)
    #endif

    #if LOG_TRACE_ENABLED
        #define RAW_TRACE(msg, ...) Raw::Logger::Get()->PrintMessage(ELogLevel::LOG_LEVEL_TRACE, msg, ## __VA_ARGS__);
    #else
        #define RAW_TRACE(msg, ...)
    #endif
#endif
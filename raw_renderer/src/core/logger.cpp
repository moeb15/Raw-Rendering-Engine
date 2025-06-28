#include "core/logger.hpp"
#include "platform/console.hpp"
#include "core/job_system.hpp"
#include "containers/thread_safe_ring_buffer.hpp"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fstream>
#include <string>

namespace Raw
{
    static Logger s_LogService;
    static constexpr u32 stringBufferSize = 32000;
    ThreadSafeRingBuffer<std::string, 8> msgQueue;
    static std::ofstream logFile;
    static cstring levelStrings[6] = 
    {
        "[FATAL]: ",
        "[ERROR]: ",
        "[WARN]: ",
        "[INFO]: ",
        "[DEBUG]: ",
        "[TRACE]: "
    };

    void AppendLogToFile(cstring msg)
    {
        logFile << msg;
    }

    Logger* Logger::Get()
    {
        return &s_LogService;
    }

    bool Logger::InititalizeLogger()
    {
        std::string dir = LOG_FILE_DIR;
        dir += "/log.txt";
        logFile.open(dir, std::ios_base::trunc);
        return true;
    }

    void Logger::Shutdown()
    {
        std::string msg;
        while(msgQueue.Pop(msg)) { AppendLogToFile(msg.c_str()); }
        logFile.close();
    }

    void Logger::PrintMessage(ELogLevel level, cstring msg, ...)
    {
        bool isError = level < ELogLevel::LOG_LEVEL_WARN;

        char outMsg[stringBufferSize];
        u64 msgSize = stringBufferSize * sizeof(char);
        memset(outMsg, 0, msgSize);

        va_list args;
        va_start(args, msg);
        vsnprintf(outMsg, sizeof(outMsg), msg, args);
        va_end(args);

        char finalMsg[stringBufferSize];
        sprintf(finalMsg, "%s%s\n", levelStrings[level], outMsg);

        
        if(isError)
        {
            PlatformConsoleWriteError(finalMsg, level);
        }else
        {
            PlatformConsoleWrite(finalMsg, level);
        }

        std::string finalStr = finalMsg;

        while(!msgQueue.Push(finalStr))
        {
            JobSystem::Execute([]()
                {
                    std::string msg;
                    while(msgQueue.Pop(msg)) { AppendLogToFile(msg.c_str()); }
                }
            );
        }
    }
}
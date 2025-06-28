#include "platform/console.hpp"

#if defined(RAW_PLATFORM_WINDOWS)

#include <Windows.h>
#include <windowsx.h>

namespace Raw
{
    void PlatformConsoleWrite(const char* msg, u8 colour)
    {
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        // fatal, error, warn, info, debug, trace
        static u8 levels[6] = {64, 4, 6, 2, 1, 8};
        SetConsoleTextAttribute(consoleHandle, levels[colour]);
        
        OutputDebugStringA(msg);
        u64 len = strlen(msg);
        LPDWORD numberWritten = 0;
        WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)len, numberWritten, 0);
    }
    
    void PlatformConsoleWriteError(const char* msg, u8 colour)
    {
        HANDLE consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        // fatal, error, warn, info, debug, trace
        static u8 levels[6] = {64, 4, 6, 2, 1, 0};
        SetConsoleTextAttribute(consoleHandle, levels[colour]);
        
        OutputDebugStringA(msg);
        u64 len = strlen(msg);
        LPDWORD numberWritten = 0;
        WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), msg, (DWORD)len, numberWritten, 0);
    }
}

#endif
#include "platform/console.hpp"

#if defined(RAW_PLATFORM_LINUX)

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev

namespace Raw
{
    void PlatformConsoleWrite(cstring msg, u8 colour)
    {
        // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
        cstring colourStrings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
        printf("\033[%sm%s\033[0m", colourStrings[colour], msg);
    }
    
    void PlatformConsoleWriteError(cstring msg, u8 colour)
    {
        // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
        cstring colourStrings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
        printf("\033[%sm%s\033[0m", colourStrings[colour], msg);
    }
}

#endif
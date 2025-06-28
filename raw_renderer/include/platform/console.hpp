#pragma once

#include "core/defines.hpp"

namespace Raw
{
    void PlatformConsoleWrite(cstring msg, u8 colour);
    void PlatformConsoleWriteError(cstring msg, u8 colour);
}
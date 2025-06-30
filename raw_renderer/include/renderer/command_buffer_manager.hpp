#pragma once

#include "renderer/command_buffer.hpp"

namespace Raw::GFX
{
    class ICommandBufferManager
    {
    public:
        virtual void Init(u32 numThreads) = 0;
        virtual void Shutdown() = 0;

        virtual void ResetBuffers(u32 frameIndex) = 0;

        virtual ICommandBuffer* GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin) = 0;
        virtual ICommandBuffer* GetSecondaryCommandBuffer(u32 frameIndex, u32 threadIndex) = 0;
    };
}
#pragma once

#include "renderer/command_buffer.hpp"
#include "renderer/gfxdevice.hpp"
#include "renderer/renderer_data.hpp"

namespace Raw::GFX
{
    class IRenderPass
    {
    public:
        virtual ~IRenderPass() {}
        
        virtual void Init(IGFXDevice* device) = 0;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) = 0;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) = 0;
    };
}
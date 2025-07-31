#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    class FXAAPass : public IRenderPass
    {
    public:
        FXAAPass() {}
        ~FXAAPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        ComputePipelineDesc techniqueDesc;
        
        TextureHandle fxaaTexture;
        TextureDesc fxaaDesc;

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;

    };
}
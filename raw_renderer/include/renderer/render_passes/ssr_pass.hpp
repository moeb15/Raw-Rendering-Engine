#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    class SSRPass : public IRenderPass
    {
    public:
        SSRPass() {}
        ~SSRPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        ComputePipelineDesc techniqueDesc;

        TextureHandle ssrTex;
        TextureDesc ssrDesc;

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;
        
    };
}
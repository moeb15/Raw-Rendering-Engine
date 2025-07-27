#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/renderer_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    class FullScreenPass : public IRenderPass
    {
    public:
        FullScreenPass() {}
        ~FullScreenPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;
        void UpdateFullScreenData();

        GPUTechnique technique;
        GraphicsPipelineDesc techiqueDesc;

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        bool OnAAToggled(const AntiAliasingToggledEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;
        EventHandler<AntiAliasingToggledEvent> m_AAHandler;
    
    };
}
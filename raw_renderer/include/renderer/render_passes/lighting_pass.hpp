#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/renderer_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    #define ILLUMINATED_SCENE "illuminated_scene"

    class LightingPass : public IRenderPass
    {
    public:
        LightingPass() {}
        ~LightingPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;
        void UpdateLightingData();

        GPUTechnique technique;
        GraphicsPipelineDesc techiqueDesc;

        TextureHandle illuminatedTexture;
        TextureDesc illuminatedDesc;

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        bool OnAOToggled(const AOToggledEvent& e);
        bool OnReflectionsToggled(const ReflectionsToggledEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;
        EventHandler<AOToggledEvent> m_AOHandler;
        EventHandler<ReflectionsToggledEvent> m_ReflectHandler;
    };
}
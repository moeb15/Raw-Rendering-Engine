#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    #define GBUFFER_DIFFUSE              "diffuse"
    #define GBUFFER_NORMAL               "normal"
    #define GBUFFER_RM_OCC               "roughMetalOcc"
    #define GBUFFER_EMISSIVE             "emissive"
    #define GBUFFER_VIEWSPACE_POS        "viewspacePosition"
    #define GBUFFER_LIGHT_CLIP_POS       "lightClipPosition"

    class GeometryPass : public IRenderPass
    {
    public:
        GeometryPass() {}
        ~GeometryPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        GraphicsPipelineDesc techiqueDesc;

        TextureHandle diffuse;
        TextureHandle normals;
        TextureHandle roughMetalOccMap;
        TextureHandle emissiveMap;
        TextureHandle viewspacePosition;
        TextureHandle lightClipSpacePosition;

        TextureDesc diffuseDesc;
        TextureDesc normalDesc;
        TextureDesc roughMetalOccDesc;
        TextureDesc emissiveDesc;
        TextureDesc viewspacePositionDesc;
        TextureDesc lightClipSpacePositionDesc;

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;

    };
}
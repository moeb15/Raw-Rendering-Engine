#pragma once

#include "renderer/render_passes/render_pass.hpp"

namespace Raw::GFX
{
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

        TextureDesc diffuseDesc;
        TextureDesc normalDesc;
        TextureDesc roughMetalOccDesc;
        TextureDesc emissiveDesc;
        TextureDesc viewspacePositionDesc;
    };
}
#pragma once

#include "renderer/render_passes/render_pass.hpp"

namespace Raw::GFX
{
    #define DIR_SHADOW_MAP "directional_shadow_map"

    class ShadowPass : public IRenderPass
    {
    public:
        ShadowPass() {}
        ~ShadowPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        GraphicsPipelineDesc techiqueDesc;

        TextureHandle shadowMap;
        TextureDesc shadowMapDesc;

    };
}
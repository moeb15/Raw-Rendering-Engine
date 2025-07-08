#pragma once

#include "renderer/render_passes/render_pass.hpp"

namespace Raw::GFX
{
    class ForwardPass : public IRenderPass
    {
    public:
        ForwardPass() {}
        ~ForwardPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        GraphicsPipelineDesc techiqueDesc;
    };
}
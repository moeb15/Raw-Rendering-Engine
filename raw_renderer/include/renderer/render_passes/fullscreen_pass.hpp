#pragma once

#include "renderer/render_passes/render_pass.hpp"

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
    };
}
#pragma once

#include "renderer/render_passes/render_pass.hpp"
#include "events/core_events.hpp"
#include "events/event_handler.hpp"

namespace Raw::GFX
{
    class FrustumCullingPass : public IRenderPass
    {
    public:
        FrustumCullingPass() {}
        ~FrustumCullingPass() {}

        virtual void Init(IGFXDevice* device) override;
        virtual void Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene) override;
        virtual void ExecuteAsync(IGFXDevice* device, SceneData* scene) override;

        GPUTechnique technique;
        ComputePipelineDesc techniqueDesc;
    };
}
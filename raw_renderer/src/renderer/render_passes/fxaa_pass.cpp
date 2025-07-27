#include "renderer/render_passes/fxaa_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static AOData data;

    void FXAAPass::Init(IGFXDevice* device)
    {
        techniqueDesc.computeShader.shaderName = "fxaa";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = 128;
        techniqueDesc.pushConstant.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.bUseDepthBuffer = true;
        techniqueDesc.imageAttachments = &device->GetDrawImageHandle();
        techniqueDesc.numImageAttachments = 1;

        techniqueDesc.name = "FXAA Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);
        data.depthBuffer = device->GetDrawImageHandle().id;
    }

    void FXAAPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(device->GetDrawImageHandle(), ETextureLayout::GENERAL);

        cmd->BindComputePipeline(technique.computePipeline);
        cmd->BindAOData(data);

        u32 groupX = device->GetBackBufferSize().first / 32;
        u32 groupY = device->GetBackBufferSize().second / 32;
        u32 groupZ = 1;
        cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);
    }

    void FXAAPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->TransitionImage(device->GetDrawImageHandle(), ETextureLayout::GENERAL);

                cmd->BindComputePipeline(technique.computePipeline);
                cmd->BindAOData(data);

                u32 groupX = device->GetBackBufferSize().first / 32;
                u32 groupY = device->GetBackBufferSize().second / 32;
                u32 groupZ = 1;
                cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);

                device->SubmitCommandBuffer(cmd);
            }
        );
    }
}
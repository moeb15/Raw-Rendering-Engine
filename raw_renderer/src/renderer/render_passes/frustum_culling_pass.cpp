#include "renderer/render_passes/frustum_culling_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    void FrustumCullingPass::Init(IGFXDevice* device)
    {
        techniqueDesc.computeShader.shaderName = "frustum_culling";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = 128;
        techniqueDesc.pushConstant.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.name = "Frustum Culling Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);
    }

    void FrustumCullingPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->BindComputePipeline(technique.computePipeline);
        cmd->BindCullData(scene->indirectBuffer, scene->meshBoundsBuffer, scene->culledIndirectBuffer, scene->drawCount);

        u32 workGroupSize = 32;
        u32 groupX = (scene->drawCount + workGroupSize - 1) / workGroupSize;
        u32 groupY = 1;
        u32 groupZ = 1;
        cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);

        cmd->AddMemoryBarrier(scene->culledIndirectBuffer, 
            EAccessFlags::SHADER_WRITE_BIT, 
            EAccessFlags::SHADER_READ_BIT, 
            EPipelineStageFlags::COMPUTE_SHADER_BIT,
            EPipelineStageFlags::VERTEX_SHADER_BIT);
    }

    void FrustumCullingPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->BindComputePipeline(technique.computePipeline);
                cmd->BindCullData(scene->indirectBuffer, scene->meshBoundsBuffer, scene->culledIndirectBuffer, scene->drawCount);

                u32 workGroupSize = 32;
                u32 groupX = (scene->drawCount + workGroupSize - 1) / workGroupSize;
                u32 groupY = 1;
                u32 groupZ = 1;
                cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);

                cmd->AddMemoryBarrier(scene->culledIndirectBuffer, 
                    EAccessFlags::SHADER_WRITE_BIT, 
                    EAccessFlags::SHADER_READ_BIT, 
                    EPipelineStageFlags::COMPUTE_SHADER_BIT,
                    EPipelineStageFlags::VERTEX_SHADER_BIT);

                device->SubmitCommandBuffer(cmd);
            }
        );
    }
}
#include "renderer/render_passes/depth_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    void DepthPass::Init(IGFXDevice* device)
    {
        techiqueDesc.sDesc.numStages = 1;
        techiqueDesc.sDesc.shaders[0].shaderName = "zpass";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;

        techiqueDesc.dsDesc.depthWriteEnable = true;
        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = device->GetMaximumPushConstantSize();
        techiqueDesc.pushConstant.stage = EShaderStage::VERTEX_STAGE;

        techiqueDesc.numImageAttachments = 0;
        techiqueDesc.depthAttachment = &device->GetDepthBufferHandle();

        techiqueDesc.name = "Depth Pre-Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);
    }

    void DepthPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
        cmd->BeginRendering(technique.gfxPipeline, ERenderingOp::LOAD, ERenderingOp::CLEAR);
        cmd->BindPipeline(technique.gfxPipeline);

        cmd->BindVertexBuffer(scene->vertexBuffer);
        cmd->BindDrawData(scene->meshDrawsBuffer);
        cmd->BindIndexBuffer(scene->indexBuffer);
        cmd->DrawIndexedIndirect(scene->indirectBuffer, 0, scene->drawCount);

        cmd->EndRendering();
        cmd->AddMemoryBarrier(
            EAccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            EAccessFlags::DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            EPipelineStageFlags::LATE_FRAGMENT_TESTS_BIT,
            EPipelineStageFlags::EARLY_FRAGMENT_TESTS_BIT);
    }

    void DepthPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
                cmd->BeginRendering(technique.gfxPipeline, ERenderingOp::LOAD, ERenderingOp::CLEAR);
                cmd->BindPipeline(technique.gfxPipeline);

                cmd->BindVertexBuffer(scene->vertexBuffer);
                cmd->BindDrawData(scene->meshDrawsBuffer);
                cmd->BindIndexBuffer(scene->indexBuffer);
                cmd->DrawIndexedIndirect(scene->indirectBuffer, 0, scene->drawCount);

                cmd->EndRendering();
                cmd->AddMemoryBarrier(
                    EAccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
                    EAccessFlags::DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    EPipelineStageFlags::LATE_FRAGMENT_TESTS_BIT,
                    EPipelineStageFlags::EARLY_FRAGMENT_TESTS_BIT);

                device->SubmitCommandBuffer(cmd);
            }
        );
    }  
}
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
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX;

        techiqueDesc.dsDesc.depthWriteEnable = true;
        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = 128;
        techiqueDesc.pushConstant.stage = EShaderStage::VERTEX;

        techiqueDesc.numImageAttachments = 0;

        techiqueDesc.name = "Depth Pre-Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);
    }

    void DepthPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
        cmd->BeginRendering(technique.gfxPipeline, true, false, true);
        cmd->BindPipeline(technique.gfxPipeline);

        for(u32 i = 0; i < scene->meshes.size(); i++)
        {
            MeshData mesh = scene->meshes[i];
            glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
        
            cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform);
            cmd->BindIndexBuffer(scene->indexBuffer);
                
            PBRMaterialData& material = scene->materials[mesh.materialIndex];
            if(material.isTransparent) continue;

            cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
        }

        cmd->EndRendering();
    }

    void DepthPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
                cmd->BeginRendering(technique.gfxPipeline, true, false, true);
                cmd->BindPipeline(technique.gfxPipeline);

                for(u32 i = 0; i < scene->meshes.size(); i++)
                {
                    MeshData mesh = scene->meshes[i];
                    glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
                
                    cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform);
                    cmd->BindIndexBuffer(scene->indexBuffer);
                        
                    PBRMaterialData& material = scene->materials[mesh.materialIndex];
                    if(material.isTransparent) continue;

                    cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
                }

                cmd->EndRendering();

                device->SubmitCommandBuffer(cmd);
            }
        );
    }  
}
#include "renderer/render_passes/shadow_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    void ShadowPass::Init(IGFXDevice* device)
    {
        shadowMapDesc.depth = 1;
        shadowMapDesc.width = SHADOW_MAP_SIZE;
        shadowMapDesc.height = SHADOW_MAP_SIZE;
        shadowMapDesc.format = ETextureFormat::D32_SFLOAT;
        shadowMapDesc.isMipmapped = false;
        shadowMapDesc.isRenderTarget = true;
        shadowMapDesc.isStorageImage = true;
        shadowMap = device->CreateTexture(shadowMapDesc, true);

        techiqueDesc.sDesc.numStages = 1;
        techiqueDesc.sDesc.shaders[0].shaderName = "shadow_pass";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;

        techiqueDesc.dsDesc.depthWriteEnable = true;
        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = 128;
        techiqueDesc.pushConstant.stage = EShaderStage::VERTEX_STAGE;

        techiqueDesc.rDesc.cullMode = ECullMode::FRONT;
        techiqueDesc.rDesc.fillMode = EFillMode::SOLID;
        techiqueDesc.rDesc.frontFace = EFrontFace::CLOCKWISE;

        techiqueDesc.numImageAttachments = 0;
        techiqueDesc.depthAttachment = &shadowMap;
        techiqueDesc.name = "Shadow Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);

        TextureLoader::Instance()->CreateFromHandle(DIR_SHADOW_MAP, shadowMap);
    }

    void ShadowPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(shadowMap, ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
        cmd->BeginRendering(technique.gfxPipeline, ERenderingOp::LOAD, ERenderingOp::CLEAR);
        cmd->BindPipeline(technique.gfxPipeline);

        for(u32 i = 0; i < scene->meshes.size(); i++)
        {
            MeshData mesh = scene->meshes[i];
            glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
        
            
            PBRMaterialData& material = scene->materials[mesh.materialIndex];
            if(material.isTransparent) continue;
            
            cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, mesh.materialIndex);
            cmd->BindIndexBuffer(scene->indexBuffer);
            cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
        }

        cmd->EndRendering();
    }

    void ShadowPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
         JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->TransitionImage(shadowMap, ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);
                cmd->BeginRendering(technique.gfxPipeline, ERenderingOp::LOAD, ERenderingOp::CLEAR);
                cmd->BindPipeline(technique.gfxPipeline);

                for(u32 i = 0; i < scene->meshes.size(); i++)
                {
                    MeshData mesh = scene->meshes[i];
                    glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
                
                    
                    PBRMaterialData& material = scene->materials[mesh.materialIndex];
                    if(material.isTransparent) continue;
                    
                    cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, mesh.materialIndex);
                    cmd->BindIndexBuffer(scene->indexBuffer);
                    cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
                }

                cmd->EndRendering();

                device->SubmitCommandBuffer(cmd);
            }
        );
    }
}
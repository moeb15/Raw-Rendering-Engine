#include "renderer/render_passes/shadow_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    void ShadowPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(ShadowPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        shadowMapDesc.depth = 1;
        shadowMapDesc.width = device->GetBackBufferSize().first;
        shadowMapDesc.height = device->GetBackBufferSize().second;
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
        cmd->BeginRendering(technique.gfxPipeline, true, false, true);
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
                cmd->BeginRendering(technique.gfxPipeline, true, false, true);
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
    
    bool ShadowPass::OnWindowResize(const WindowResizeEvent& e)
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(DIR_SHADOW_MAP);

        shadowMapDesc.width = e.GetWidth();
        shadowMapDesc.height = e.GetHeight();

        shadowMap = device->CreateTexture(shadowMapDesc, true);

        TextureLoader::Instance()->CreateFromHandle(DIR_SHADOW_MAP, shadowMap);

        device->UpdateGraphicsPipelineDepthAttachment(technique.gfxPipeline, &shadowMap);

        return false;
    }
}
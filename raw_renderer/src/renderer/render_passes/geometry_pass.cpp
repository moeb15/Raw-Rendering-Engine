#include "renderer/render_passes/geometry_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static TextureHandle defaultTexture;
    static TextureHandle defaultEmissive;
    static TextureHandle errorTexture;
    static TextureHandle geometryPassImages[5];

    void GeometryPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(GeometryPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        diffuseDesc.depth = 1;
        diffuseDesc.width = windowSize.first;
        diffuseDesc.height = windowSize.second;
        diffuseDesc.isMipmapped = false;
        diffuseDesc.isStorageImage = true;
        diffuseDesc.isRenderTarget = true;
        diffuseDesc.type = ETextureType::TEXTURE2D;
        diffuseDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        diffuse = device->CreateTexture(diffuseDesc);

        normalDesc.depth = 1;
        normalDesc.width = windowSize.first;
        normalDesc.height = windowSize.second;
        normalDesc.isMipmapped = false;
        normalDesc.isStorageImage = true;
        normalDesc.isRenderTarget = true;
        normalDesc.type = ETextureType::TEXTURE2D;
        normalDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        normals = device->CreateTexture(normalDesc);

        roughMetalOccDesc.depth = 1;
        roughMetalOccDesc.width = windowSize.first;
        roughMetalOccDesc.height = windowSize.second;
        roughMetalOccDesc.isMipmapped = false;
        roughMetalOccDesc.isStorageImage = true;
        roughMetalOccDesc.isRenderTarget = true;
        roughMetalOccDesc.type = ETextureType::TEXTURE2D;
        roughMetalOccDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        roughMetalOccMap = device->CreateTexture(normalDesc);

        emissiveDesc.depth = 1;
        emissiveDesc.width = windowSize.first;
        emissiveDesc.height = windowSize.second;
        emissiveDesc.isMipmapped = false;
        emissiveDesc.isStorageImage = true;
        emissiveDesc.isRenderTarget = true;
        emissiveDesc.type = ETextureType::TEXTURE2D;
        emissiveDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        emissiveMap = device->CreateTexture(emissiveDesc);

        viewspacePositionDesc.depth = 1;
        viewspacePositionDesc.width = windowSize.first;
        viewspacePositionDesc.height = windowSize.second;
        viewspacePositionDesc.isMipmapped = false;
        viewspacePositionDesc.isStorageImage = true;
        viewspacePositionDesc.isRenderTarget = true;
        viewspacePositionDesc.type = ETextureType::TEXTURE2D;
        viewspacePositionDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        viewspacePosition = device->CreateTexture(viewspacePositionDesc);

        techiqueDesc.sDesc.numStages = 2;
        techiqueDesc.sDesc.shaders[0].shaderName = "gbuffer";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;
        techiqueDesc.sDesc.shaders[1].shaderName = "gbuffer";
        techiqueDesc.sDesc.shaders[1].stage = EShaderStage::FRAGMENT_STAGE;

        techiqueDesc.dsDesc.depthWriteEnable = true;
        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = 128;
        techiqueDesc.pushConstant.stage = EShaderStage::VERTEX_STAGE;

        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthWriteEnable = true;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS;
        
        techiqueDesc.rDesc.cullMode = ECullMode::BACK;
        techiqueDesc.rDesc.fillMode = EFillMode::SOLID;
        techiqueDesc.rDesc.frontFace = EFrontFace::CLOCKWISE;
        
        techiqueDesc.bsDesc[0].blendEnabled = true;
        techiqueDesc.bsDesc[0].seperateBlend = true;
        techiqueDesc.bsDesc[0].srcColour = EBlendState::SRC_ALPHA;
        techiqueDesc.bsDesc[0].dstColour = EBlendState::ONE_MINUS_SRC_ALPHA;
        techiqueDesc.bsDesc[0].colourOp = EBlendOp::ADD;
        techiqueDesc.bsDesc[0].srcAlpha = EBlendState::ONE;
        techiqueDesc.bsDesc[0].dstAlpha = EBlendState::ZERO;
        techiqueDesc.bsDesc[0].alphaOp = EBlendOp::ADD;

        techiqueDesc.numImageAttachments = 5;
        geometryPassImages[0] = diffuse;
        geometryPassImages[1] = normals;
        geometryPassImages[2] = roughMetalOccMap;
        geometryPassImages[3] = emissiveMap;
        geometryPassImages[4] = viewspacePosition;
        techiqueDesc.imageAttachments = geometryPassImages;
        techiqueDesc.name = "Geometry Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);
        
        TextureResource* tex = (TextureResource*)TextureLoader::Instance()->Get(ERROR_TEXTURE);
        errorTexture = tex->handle;
        tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_TEXTURE);
        defaultTexture = tex->handle;
        tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_EMISSIVE);
        defaultEmissive = tex->handle;

        TextureLoader::Instance()->CreateFromHandle(GBUFFER_DIFFUSE, diffuse);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_NORMAL, normals);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_RM_OCC, roughMetalOccMap);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_EMISSIVE, emissiveMap);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_VIEWSPACE_POS, viewspacePosition);
    }

    void GeometryPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL);

        cmd->BeginRendering(technique.gfxPipeline, true, true, true);
        cmd->BindPipeline(technique.gfxPipeline);

        for(u32 i = 0; i < scene->meshes.size(); i++)
        {
            MeshData mesh = scene->meshes[i];
            glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
            PBRMaterialData material = scene->materials[mesh.materialIndex];
            if(material.isTransparent) continue;
    
            cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, mesh.materialIndex);
            cmd->BindIndexBuffer(scene->indexBuffer);
            cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
        }

        cmd->EndRendering();
    }

    
    void GeometryPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
         JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();

                cmd->BeginRendering(technique.gfxPipeline, true, true, true);
                cmd->BindPipeline(technique.gfxPipeline);
                for(u32 i = 0; i < scene->meshes.size(); i++)
                {
                    MeshData mesh = scene->meshes[i];
                    glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
                    PBRMaterialData material = scene->materials[mesh.materialIndex];
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
    
    bool GeometryPass::OnWindowResize(const WindowResizeEvent& e)
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(GBUFFER_DIFFUSE);
        TextureLoader::Instance()->Remove(GBUFFER_NORMAL);
        TextureLoader::Instance()->Remove(GBUFFER_RM_OCC);
        TextureLoader::Instance()->Remove(GBUFFER_EMISSIVE);
        TextureLoader::Instance()->Remove(GBUFFER_VIEWSPACE_POS);

        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        diffuseDesc.width = windowSize.first;
        diffuseDesc.height = windowSize.second;

        normalDesc.width = windowSize.first;
        normalDesc.height = windowSize.second;

        roughMetalOccDesc.width = windowSize.first;
        roughMetalOccDesc.height = windowSize.second;

        emissiveDesc.width = windowSize.first;
        emissiveDesc.height = windowSize.second;

        viewspacePositionDesc.width = windowSize.first;
        viewspacePositionDesc.height = windowSize.second;

        diffuse = device->CreateTexture(diffuseDesc);
        normals = device->CreateTexture(normalDesc);
        roughMetalOccMap = device->CreateTexture(roughMetalOccDesc);
        emissiveMap = device->CreateTexture(emissiveDesc);
        viewspacePosition = device->CreateTexture(viewspacePositionDesc);

        TextureLoader::Instance()->CreateFromHandle(GBUFFER_DIFFUSE, diffuse);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_NORMAL, normals);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_RM_OCC, roughMetalOccMap);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_EMISSIVE, emissiveMap);
        TextureLoader::Instance()->CreateFromHandle(GBUFFER_VIEWSPACE_POS, viewspacePosition);

        geometryPassImages[0] = diffuse;
        geometryPassImages[1] = normals;
        geometryPassImages[2] = roughMetalOccMap;
        geometryPassImages[3] = emissiveMap;
        geometryPassImages[4] = viewspacePosition;

        device->UpdateGraphicsPipelineImageAttachments(technique.gfxPipeline, ArraySize(geometryPassImages), geometryPassImages);

        return false;
    }
}
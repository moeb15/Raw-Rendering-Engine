#include "renderer/render_passes/forward_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static TextureHandle defaultTexture;
    static TextureHandle defaultEmissive;
    static TextureHandle errorTexture;

    void ForwardPass::Init(IGFXDevice* device)
    {
        techiqueDesc.sDesc.numStages = 2;
        techiqueDesc.sDesc.shaders[0].shaderName = "pbr_forward";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX;
        techiqueDesc.sDesc.shaders[1].shaderName = "pbr_forward";
        techiqueDesc.sDesc.shaders[1].stage = EShaderStage::FRAGMENT;
        
        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = 128;
        techiqueDesc.pushConstant.stage = GFX::EShaderStage::VERTEX;

        techiqueDesc.dsDesc.depthEnable = true;
        techiqueDesc.dsDesc.stencilEnable = false;
        techiqueDesc.dsDesc.depthWriteEnable = false;
        techiqueDesc.dsDesc.depthComparison = EComparisonFunc::LESS_EQUAL;
        
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

        techiqueDesc.numImageAttachments = 1;
        TextureHandle* forwardPassImages = &device->GetDrawImageHandle();
        techiqueDesc.imageAttachments = forwardPassImages;

        techiqueDesc.name = "Forward Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);

        TextureResource* tex = (TextureResource*)TextureLoader::Instance()->Get(ERROR_TEXTURE);
        errorTexture = tex->handle;
        tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_TEXTURE);
        defaultTexture = tex->handle;
        tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_EMISSIVE);
        defaultEmissive = tex->handle;
    }

    void ForwardPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->BeginRendering(technique.gfxPipeline, true, true, false);
        cmd->BindPipeline(technique.gfxPipeline);

        for(u32 i = 0; i < scene->meshes.size(); i++)
        {
            MeshData mesh = scene->meshes[i];
            glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
            PBRMaterialData& material = scene->materials[mesh.materialIndex];
            if(material.isTransparent) continue;
        
            u32 diffuse = errorTexture.id;
            u32 normal = defaultTexture.id;
            u32 roughness = defaultTexture.id;
            u32 occlusion = defaultTexture.id;
            u32 emissive = defaultEmissive.id;

            if(material.diffuse != -1)      diffuse = scene->images[scene->textures[material.diffuse]];
            if(material.normal != -1)       normal = scene->images[scene->textures[material.normal]];
            if(material.roughness != -1)    roughness = scene->images[scene->textures[material.roughness]];
            if(material.occlusion != -1)    occlusion = scene->images[scene->textures[material.occlusion]];
            if(material.emissive != -1)     emissive = scene->images[scene->textures[material.emissive]];

            PBRMaterialData renderedMat = material;
            renderedMat.diffuse = diffuse;
            renderedMat.roughness = roughness;
            renderedMat.occlusion = occlusion;
            renderedMat.normal = normal;
            renderedMat.emissive = emissive;
            
            cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, renderedMat);
            cmd->BindIndexBuffer(scene->indexBuffer);
            cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
        }

        for(u32 i = 0; i < scene->meshes.size(); i++)
        {
            MeshData mesh = scene->meshes[i];
            glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
            PBRMaterialData& material = scene->materials[mesh.materialIndex];
            if(!material.isTransparent) continue;
        
            u32 diffuse = errorTexture.id;
            u32 normal = defaultTexture.id;
            u32 roughness = defaultTexture.id;
            u32 occlusion = defaultTexture.id;
            u32 emissive = defaultEmissive.id;

            if(material.diffuse != -1)      diffuse = scene->images[scene->textures[material.diffuse]];
            if(material.normal != -1)       normal = scene->images[scene->textures[material.normal]];
            if(material.roughness != -1)    roughness = scene->images[scene->textures[material.roughness]];
            if(material.occlusion != -1)    occlusion = scene->images[scene->textures[material.occlusion]];
            if(material.emissive != -1)     emissive = scene->images[scene->textures[material.emissive]];

            PBRMaterialData renderedMat = material;
            renderedMat.diffuse = diffuse;
            renderedMat.roughness = roughness;
            renderedMat.occlusion = occlusion;
            renderedMat.normal = normal;
            renderedMat.emissive = emissive;
            
            cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, renderedMat);
            cmd->BindIndexBuffer(scene->indexBuffer);
            cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
        }

        cmd->EndRendering();
    }

    void ForwardPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();

                cmd->BeginRendering(technique.gfxPipeline, true, true, false);
                cmd->BindPipeline(technique.gfxPipeline);

                for(u32 i = 0; i < scene->meshes.size(); i++)
                {
                    MeshData mesh = scene->meshes[i];
                    glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
                    PBRMaterialData& material = scene->materials[mesh.materialIndex];
                    if(material.isTransparent) continue;
                
                    u32 diffuse = errorTexture.id;
                    u32 normal = defaultTexture.id;
                    u32 roughness = defaultTexture.id;
                    u32 occlusion = defaultTexture.id;
                    u32 emissive = defaultEmissive.id;

                    if(material.diffuse != -1)      diffuse = scene->images[scene->textures[material.diffuse]];
                    if(material.normal != -1)       normal = scene->images[scene->textures[material.normal]];
                    if(material.roughness != -1)    roughness = scene->images[scene->textures[material.roughness]];
                    if(material.occlusion != -1)    occlusion = scene->images[scene->textures[material.occlusion]];
                    if(material.emissive != -1)     emissive = scene->images[scene->textures[material.emissive]];

                    PBRMaterialData renderedMat = material;
                    renderedMat.diffuse = diffuse;
                    renderedMat.roughness = roughness;
                    renderedMat.occlusion = occlusion;
                    renderedMat.normal = normal;
                    renderedMat.emissive = emissive;
                    
                    cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, renderedMat);
                    cmd->BindIndexBuffer(scene->indexBuffer);
                    cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
                }

                for(u32 i = 0; i < scene->meshes.size(); i++)
                {
                    MeshData mesh = scene->meshes[i];
                    glm::mat4 meshTransform = scene->transforms[mesh.transformIndex];
                    PBRMaterialData& material = scene->materials[mesh.materialIndex];
                    if(!material.isTransparent) continue;
                
                    u32 diffuse = errorTexture.id;
                    u32 normal = defaultTexture.id;
                    u32 roughness = defaultTexture.id;
                    u32 occlusion = defaultTexture.id;
                    u32 emissive = defaultEmissive.id;

                    if(material.diffuse != -1)      diffuse = scene->images[scene->textures[material.diffuse]];
                    if(material.normal != -1)       normal = scene->images[scene->textures[material.normal]];
                    if(material.roughness != -1)    roughness = scene->images[scene->textures[material.roughness]];
                    if(material.occlusion != -1)    occlusion = scene->images[scene->textures[material.occlusion]];
                    if(material.emissive != -1)     emissive = scene->images[scene->textures[material.emissive]];

                    PBRMaterialData renderedMat = material;
                    renderedMat.diffuse = diffuse;
                    renderedMat.roughness = roughness;
                    renderedMat.occlusion = occlusion;
                    renderedMat.normal = normal;
                    renderedMat.emissive = emissive;
                    
                    cmd->BindVertexBuffer(scene->vertexBuffer, meshTransform, renderedMat);
                    cmd->BindIndexBuffer(scene->indexBuffer);
                    cmd->DrawIndexed(mesh.indexCount, mesh.instanceCount, mesh.firstIndex, mesh.vertexOffset, mesh.baseInstance);
                }

                cmd->EndRendering();

                device->SubmitCommandBuffer(cmd);
            }
        );
    }  
}
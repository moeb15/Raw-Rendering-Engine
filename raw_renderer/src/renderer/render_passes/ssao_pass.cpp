#include "renderer/render_passes/ssao_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static TextureHandle ssaoImages[2];

    void SSAOPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(SSAOPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        occlusionDesc.depth = 1;
        occlusionDesc.width = windowSize.first;
        occlusionDesc.height = windowSize.second;
        occlusionDesc.isMipmapped = false;
        occlusionDesc.isStorageImage = true;
        occlusionDesc.isRenderTarget = true;
        occlusionDesc.type = ETextureType::TEXTURE2D;
        occlusionDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        occlusionTex = device->CreateTexture(occlusionDesc);

        techniqueDesc.computeShader.shaderName = "ssao";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        ssaoImages[0] = device->GetDepthBufferHandle();
        ssaoImages[1] = occlusionTex;
        techniqueDesc.numStorageImages = 1;
        techniqueDesc.storageImages = &ssaoImages[1];

        techniqueDesc.numCombinedImageSamples = 1;
        techniqueDesc.combinedImageSamples = &ssaoImages[0];

        techniqueDesc.layoutDesc.setIndex = 0;
        techniqueDesc.layoutDesc.numBindings = 2;
        
        techniqueDesc.layoutDesc.bindings[0].bind = 0;
        techniqueDesc.layoutDesc.bindings[0].count = 1;
        techniqueDesc.layoutDesc.bindings[0].stage = EShaderStage::COMPUTE_STAGE;
        techniqueDesc.layoutDesc.bindings[0].type = EDescriptorType::STORAGE_IMAGE;
        
        techniqueDesc.layoutDesc.bindings[1].bind = 1;
        techniqueDesc.layoutDesc.bindings[1].count = 1;
        techniqueDesc.layoutDesc.bindings[1].stage = EShaderStage::COMPUTE_STAGE;
        techniqueDesc.layoutDesc.bindings[1].type = EDescriptorType::IMAGE_SAMPLER;

        techniqueDesc.name = "SSAO Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);

        TextureLoader::Instance()->CreateFromHandle(AMBIENT_OCCLUSION_TEX, occlusionTex);
    }

    void SSAOPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(occlusionTex, ETextureLayout::GENERAL);
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::GENERAL);

        u32 groupX = device->GetBackBufferSize().first / 16;
        u32 groupY = device->GetBackBufferSize().second / 16;
        u32 groupZ = 1;
        cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);
    }

    void SSAOPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->TransitionImage(occlusionTex, ETextureLayout::GENERAL);
                cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::GENERAL);

                u32 groupX = device->GetBackBufferSize().first / 16;
                u32 groupY = device->GetBackBufferSize().second / 16;
                u32 groupZ = 1;
                cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);

                device->SubmitCommandBuffer(cmd);
            }
        );
    }
    
    bool SSAOPass::OnWindowResize(const WindowResizeEvent& e)
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(AMBIENT_OCCLUSION_TEX);

        occlusionDesc.width = e.GetWidth();
        occlusionDesc.height = e.GetHeight();

        occlusionTex = device->CreateTexture(occlusionDesc);
        ssaoImages[1] = occlusionTex;

        TextureLoader::Instance()->CreateFromHandle(AMBIENT_OCCLUSION_TEX, occlusionTex);

        Binding bindings[2];
        bindings[0].bind = 1;
        bindings[0].count = 1;
        bindings[0].stage = EShaderStage::COMPUTE_STAGE;
        bindings[0].type = EDescriptorType::IMAGE_SAMPLER;

        bindings[1].bind = 0;
        bindings[1].count = 1;
        bindings[1].stage = EShaderStage::COMPUTE_STAGE;
        bindings[1].type = EDescriptorType::STORAGE_IMAGE;

        device->UpdateComputePipelineDescriptorSet(technique.computePipeline, ArraySize(ssaoImages), bindings, ssaoImages);

        return false;
    }
}
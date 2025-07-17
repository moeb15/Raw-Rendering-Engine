#include "renderer/render_passes/ssao_pass.hpp"
#include "renderer/render_passes/geometry_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static TextureHandle ssaoImages[1];
    static AOData data;

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
        occlusionDesc.isSampledImage = true;
        occlusionDesc.type = ETextureType::TEXTURE2D;
        occlusionDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        occlusionTex = device->CreateTexture(occlusionDesc);

        techniqueDesc.computeShader.shaderName = "ssao";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = 128;
        techniqueDesc.pushConstant.stage = EShaderStage::COMPUTE_STAGE;

        ssaoImages[0] = occlusionTex;
        techniqueDesc.bUseDepthBuffer = true;
        techniqueDesc.imageAttachments = ssaoImages;
        techniqueDesc.numImageAttachments = ArraySize(ssaoImages);

        techniqueDesc.name = "SSAO Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);

        TextureLoader::Instance()->CreateFromHandle(AMBIENT_OCCLUSION_TEX, occlusionTex);
        
        data.outputAOTexture = occlusionTex.id;
        data.depthBuffer = device->GetDepthBufferHandle().id;
        data.normalBuffer = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_NORMAL))->handle.id;
    }

    void SSAOPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(occlusionTex, ETextureLayout::GENERAL);
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::GENERAL);

        cmd->BindComputePipeline(technique.computePipeline);
        cmd->BindAOData(data);

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

                cmd->BindComputePipeline(technique.computePipeline);
                cmd->BindAOData(data);

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
        ssaoImages[0] = occlusionTex;

        TextureLoader::Instance()->CreateFromHandle(AMBIENT_OCCLUSION_TEX, occlusionTex);

        device->UpdateComputePipelineDescriptorSet(technique.computePipeline, ArraySize(ssaoImages), ssaoImages);

        data.outputAOTexture = occlusionTex.id;
        data.depthBuffer = device->GetDepthBufferHandle().id;
        data.normalBuffer = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_NORMAL))->handle.id;

        return false;
    }
}
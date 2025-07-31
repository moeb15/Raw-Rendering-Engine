#include "renderer/render_passes/fxaa_pass.hpp"
#include "renderer/render_passes/lighting_pass.hpp"
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
        m_ResizeHandler = BIND_EVENT_FN(FXAAPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        fxaaDesc.depth = 1;
        fxaaDesc.width = windowSize.first;
        fxaaDesc.height = windowSize.second;
        fxaaDesc.isMipmapped = false;
        fxaaDesc.isStorageImage = true;
        fxaaDesc.isRenderTarget = true;
        fxaaDesc.isSampledImage = true;
        fxaaDesc.type = ETextureType::TEXTURE2D;
        fxaaDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        fxaaTexture = device->CreateTexture(fxaaDesc);

        techniqueDesc.computeShader.shaderName = "fxaa";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = 128;
        techniqueDesc.pushConstant.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.bUseDepthBuffer = true;
        techniqueDesc.imageAttachments = &fxaaTexture;
        techniqueDesc.numImageAttachments = 1;

        techniqueDesc.name = "FXAA Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);
        TextureLoader::Instance()->CreateFromHandle(ANTI_ALIASING_TEX, fxaaTexture);

        data.depthBuffer = ((TextureResource*)TextureLoader::Instance()->Get(ILLUMINATED_SCENE))->handle.id;
        data.outputAOTexture = fxaaTexture.id;
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

    bool FXAAPass::OnWindowResize(const WindowResizeEvent& e)
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(ANTI_ALIASING_TEX);

        fxaaDesc.width = e.GetWidth();
        fxaaDesc.height = e.GetHeight();

        fxaaTexture = device->CreateTexture(fxaaDesc);

        TextureLoader::Instance()->CreateFromHandle(ANTI_ALIASING_TEX, fxaaTexture);

        device->UpdateComputePipelineDescriptorSet(technique.computePipeline, 1, &fxaaTexture);

        data.depthBuffer = ((TextureResource*)TextureLoader::Instance()->Get(ILLUMINATED_SCENE))->handle.id;
        data.outputAOTexture = fxaaTexture.id;

        return false;
    }
}
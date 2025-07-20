#include "renderer/render_passes/ssr_pass.hpp"
#include "renderer/render_passes/geometry_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static TextureHandle ssrImages[1];
    static AOData data;

    void SSRPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(SSRPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        ssrDesc.depth = 1;
        ssrDesc.width = windowSize.first;
        ssrDesc.height = windowSize.second;
        ssrDesc.isMipmapped = false;
        ssrDesc.isStorageImage = true;
        ssrDesc.isRenderTarget = true;
        ssrDesc.isSampledImage = true;
        ssrDesc.type = ETextureType::TEXTURE2D;
        ssrDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        ssrTex = device->CreateTexture(ssrDesc);

        techniqueDesc.computeShader.shaderName = "ssr";
        techniqueDesc.computeShader.stage = EShaderStage::COMPUTE_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = 128;
        techniqueDesc.pushConstant.stage = EShaderStage::COMPUTE_STAGE;

        ssrImages[0] = ssrTex;
        techniqueDesc.bUseDepthBuffer = true;
        techniqueDesc.imageAttachments = ssrImages;
        techniqueDesc.numImageAttachments = ArraySize(ssrImages);

        techniqueDesc.name = "SSR Pass";

        technique.computePipeline = device->CreateComputePipeline(techniqueDesc);

        TextureLoader::Instance()->CreateFromHandle(REFLECTION_TEX, ssrTex);
        
        data.outputAOTexture = ssrTex.id;
        data.depthBuffer = device->GetDepthBufferHandle().id;
        data.normalBuffer = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_NORMAL))->handle.id;
        data.diffuse = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_DIFFUSE))->handle.id;
        data.metallicRoughness = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_RM_OCC))->handle.id;
    }

    void SSRPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(ssrTex, ETextureLayout::GENERAL);
        cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::GENERAL);
        cmd->TransitionImage({ (u32)data.normalBuffer }, ETextureLayout::GENERAL);

        cmd->BindComputePipeline(technique.computePipeline);
        cmd->BindAOData(data);

        u32 workGroupSize = 16;
        u32 groupX = (device->GetBackBufferSize().first + workGroupSize - 1) / workGroupSize;
        u32 groupY = (device->GetBackBufferSize().second + workGroupSize - 1)/ workGroupSize;
        u32 groupZ = 1;
        cmd->Dispatch(technique.computePipeline, groupX, groupY, groupZ);
    }

    void SSRPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->TransitionImage(ssrTex, ETextureLayout::GENERAL);
                cmd->TransitionImage(device->GetDepthBufferHandle(), ETextureLayout::GENERAL);
                cmd->TransitionImage({ (u32)data.normalBuffer }, ETextureLayout::GENERAL);

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
    
    bool SSRPass::OnWindowResize(const WindowResizeEvent& e)
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(REFLECTION_TEX);

        ssrDesc.width = e.GetWidth();
        ssrDesc.height = e.GetHeight();

        ssrTex = device->CreateTexture(ssrDesc);
        ssrImages[0] = ssrTex;

        TextureLoader::Instance()->CreateFromHandle(REFLECTION_TEX, ssrTex);

        device->UpdateComputePipelineDescriptorSet(technique.computePipeline, ArraySize(ssrImages), ssrImages);

        data.outputAOTexture = ssrTex.id;
        data.depthBuffer = device->GetDepthBufferHandle().id;
        data.normalBuffer = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_NORMAL))->handle.id;
        data.diffuse = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_DIFFUSE))->handle.id;
        data.metallicRoughness = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_RM_OCC))->handle.id;

        return false;
    }
}
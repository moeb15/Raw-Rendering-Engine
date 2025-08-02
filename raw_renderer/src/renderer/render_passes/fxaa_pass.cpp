#include "renderer/render_passes/fxaa_pass.hpp"
#include "renderer/render_passes/lighting_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static FullScreenData data;

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

        techniqueDesc.sDesc.numStages = 2;
        techniqueDesc.sDesc.shaders[0].shaderName = "fullscreen";
        techniqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;
        techniqueDesc.sDesc.shaders[1].shaderName = "fxaa";
        techniqueDesc.sDesc.shaders[1].stage = EShaderStage::FRAGMENT_STAGE;

        techniqueDesc.pushConstant.offset = 0;
        techniqueDesc.pushConstant.size = device->GetMaximumPushConstantSize();
        techniqueDesc.pushConstant.stage = EShaderStage::FRAGMENT_STAGE;

        techniqueDesc.dsDesc.depthEnable = false;
        techniqueDesc.rDesc.cullMode = ECullMode::BACK;
        techniqueDesc.rDesc.fillMode = EFillMode::SOLID;
        techniqueDesc.rDesc.frontFace = EFrontFace::CLOCKWISE;

        techniqueDesc.numImageAttachments = 1;
        techniqueDesc.imageAttachments = &fxaaTexture;

        techniqueDesc.name = "FXAA Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techniqueDesc);
        TextureLoader::Instance()->CreateFromHandle(ANTI_ALIASING_TEX, fxaaTexture);

        data.diffuse = ((TextureResource*)TextureLoader::Instance()->Get(ILLUMINATED_SCENE))->handle.id;
    }

    void FXAAPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->TransitionImage(device->GetDrawImageHandle(), ETextureLayout::GENERAL);

        cmd->BeginRendering(technique.gfxPipeline);
        cmd->BindPipeline(technique.gfxPipeline);
        cmd->BindFullScreenData(data);

        cmd->Draw(3, 1, 0, 0);

        cmd->EndRendering();
    }

    void FXAAPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
                ICommandBuffer* cmd = device->GetCommandBuffer();
                cmd->BeginCommandBuffer();
                cmd->TransitionImage(device->GetDrawImageHandle(), ETextureLayout::GENERAL);

                cmd->BeginRendering(technique.gfxPipeline);
                cmd->BindPipeline(technique.gfxPipeline);
                cmd->BindFullScreenData(data);

                cmd->Draw(3, 1, 0, 0);

                cmd->EndRendering();

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

        device->UpdateGraphicsPipelineImageAttachments(technique.gfxPipeline, 1, &fxaaTexture);

        data.diffuse = ((TextureResource*)TextureLoader::Instance()->Get(ILLUMINATED_SCENE))->handle.id;

        return false;
    }
}
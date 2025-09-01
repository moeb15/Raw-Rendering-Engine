#include "renderer/render_passes/lighting_pass.hpp"
#include "renderer/render_passes/geometry_pass.hpp"
#include "renderer/render_passes/transparency_pass.hpp"
#include "core/servicelocator.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "events/event_manager.hpp"
#include "core/job_system.hpp"

namespace Raw::GFX
{
    static FullScreenData data;
    static bool ssrToggled = true;

    void LightingPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(LightingPass::OnWindowResize);
        m_AOHandler = BIND_EVENT_FN(LightingPass::OnAOToggled);
        m_ReflectHandler = BIND_EVENT_FN(LightingPass::OnReflectionsToggled);

        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_AOHandler, AOToggledEvent), AOToggledEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ReflectHandler, ReflectionsToggledEvent), ReflectionsToggledEvent::GetStaticEventType());


        std::pair<u32, u32> windowSize = device->GetBackBufferSize();
        illuminatedDesc.depth = 1;
        illuminatedDesc.width = windowSize.first;
        illuminatedDesc.height = windowSize.second;
        illuminatedDesc.isMipmapped = false;
        illuminatedDesc.isStorageImage = true;
        illuminatedDesc.isRenderTarget = true;
        illuminatedDesc.isSampledImage = true;
        illuminatedDesc.type = ETextureType::TEXTURE2D;
        illuminatedDesc.format = ETextureFormat::R8G8B8A8_UNORM;
        illuminatedTexture = device->CreateTexture(illuminatedDesc);

        techiqueDesc.sDesc.numStages = 2;
        techiqueDesc.sDesc.shaders[0].shaderName = "fullscreen";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;
        techiqueDesc.sDesc.shaders[1].shaderName = "lighting";
        techiqueDesc.sDesc.shaders[1].stage = EShaderStage::FRAGMENT_STAGE;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = device->GetMaximumPushConstantSize();
        techiqueDesc.pushConstant.stage = EShaderStage::FRAGMENT_STAGE;

        techiqueDesc.dsDesc.depthEnable = false;
        techiqueDesc.rDesc.cullMode = ECullMode::BACK;
        techiqueDesc.rDesc.fillMode = EFillMode::SOLID;
        techiqueDesc.rDesc.frontFace = EFrontFace::CLOCKWISE;

        techiqueDesc.numImageAttachments = 1;
        techiqueDesc.imageAttachments = &illuminatedTexture;

        techiqueDesc.name = "Lighting Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);

        TextureLoader::Instance()->CreateFromHandle(ILLUMINATED_SCENE, illuminatedTexture);
    }

    void LightingPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->BeginRendering(technique.gfxPipeline);
        cmd->BindPipeline(technique.gfxPipeline);

        cmd->BindFullScreenData(data);
        cmd->Draw(3, 1, 0, 0);
        
        cmd->EndRendering();
    }

    void LightingPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
            }
        );
    }  

    void LightingPass::UpdateLightingData()
    {
        TextureResource* tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_DIFFUSE);
        data.diffuse = tex->handle.id;

        tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_NORMAL);
        data.normal = tex->handle.id;

        tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_RM_OCC);
        data.roughness = tex->handle.id;
        
        tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_EMISSIVE);
        data.emissive = tex->handle.id;
        
        tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_VIEWSPACE_POS);
        data.viewspace = tex->handle.id;

        tex = (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_LIGHT_CLIP_POS);
        data.lightClipSpacePos = tex->handle.id;

        tex = (TextureResource*)TextureLoader::Instance()->Get(TPASS_TEX);
        data.transparent = tex->handle.id;

        tex = (TextureResource*)TextureLoader::Instance()->Get(AMBIENT_OCCLUSION_TEX);
        data.occlusion = tex->handle.id;

        tex = ssrToggled ? (TextureResource*)TextureLoader::Instance()->Get(REFLECTION_TEX) : (TextureResource*)TextureLoader::Instance()->Get(GBUFFER_DIFFUSE);
        data.reflection = tex->handle.id;
    }

    bool LightingPass::OnWindowResize(const WindowResizeEvent& e)
    {
        UpdateLightingData();

        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        TextureLoader::Instance()->Remove(ILLUMINATED_SCENE);

        illuminatedDesc.width = e.GetWidth();
        illuminatedDesc.height = e.GetHeight();
        
        illuminatedTexture = device->CreateTexture(illuminatedDesc);

        TextureLoader::Instance()->CreateFromHandle(ILLUMINATED_SCENE, illuminatedTexture);

        device->UpdateGraphicsPipelineImageAttachments(technique.gfxPipeline, 1, &illuminatedTexture);

        return false;
    }

    bool LightingPass::OnAOToggled(const AOToggledEvent& e)
    {
        if(e.GetState())
        {
            data.occlusion = ((TextureResource*)TextureLoader::Instance()->Get(AMBIENT_OCCLUSION_TEX))->handle.id;
        }
        else
        {
            data.occlusion = ((TextureResource*)TextureLoader::Instance()->Get(DEFAULT_TEXTURE))->handle.id;
        }

        return true;
    }

    bool LightingPass::OnReflectionsToggled(const ReflectionsToggledEvent& e)
    {
        if(e.GetState())
        {
            data.reflection = ((TextureResource*)TextureLoader::Instance()->Get(REFLECTION_TEX))->handle.id;
        }
        else
        {
            data.reflection = ((TextureResource*)TextureLoader::Instance()->Get(GBUFFER_DIFFUSE))->handle.id;
        }

        ssrToggled = e.GetState();
        return true;
    }
}
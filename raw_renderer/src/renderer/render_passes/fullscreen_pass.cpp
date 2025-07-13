#include "renderer/render_passes/fullscreen_pass.hpp"
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

    void FullScreenPass::Init(IGFXDevice* device)
    {
        m_ResizeHandler = BIND_EVENT_FN(FullScreenPass::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());

        techiqueDesc.sDesc.numStages = 2;
        techiqueDesc.sDesc.shaders[0].shaderName = "fullscreen";
        techiqueDesc.sDesc.shaders[0].stage = EShaderStage::VERTEX_STAGE;
        techiqueDesc.sDesc.shaders[1].shaderName = "fullscreen";
        techiqueDesc.sDesc.shaders[1].stage = EShaderStage::FRAGMENT_STAGE;

        techiqueDesc.pushConstant.offset = 0;
        techiqueDesc.pushConstant.size = device->GetMaximumPushConstantSize();
        techiqueDesc.pushConstant.stage = EShaderStage::FRAGMENT_STAGE;

        techiqueDesc.dsDesc.depthEnable = false;
        techiqueDesc.rDesc.cullMode = ECullMode::BACK;
        techiqueDesc.rDesc.fillMode = EFillMode::SOLID;
        techiqueDesc.rDesc.frontFace = EFrontFace::CLOCKWISE;

        techiqueDesc.numImageAttachments = 1;
        techiqueDesc.imageAttachments = &device->GetDrawImageHandle();

        techiqueDesc.name = "FullScreen Pass";

        technique.gfxPipeline = device->CreateGraphicsPipeline(techiqueDesc);
    }

    void FullScreenPass::Execute(IGFXDevice* device, ICommandBuffer* cmd, SceneData* scene)
    {
        cmd->BeginRendering(technique.gfxPipeline);
        cmd->BindPipeline(technique.gfxPipeline);

        cmd->BindFullScreenData(data);
        cmd->Draw(3, 1, 0, 0);
        
        cmd->EndRendering();
    }

    void FullScreenPass::ExecuteAsync(IGFXDevice* device, SceneData* scene)
    {
        JobSystem::Execute([&]()
            {
            }
        );
    }  

    void FullScreenPass::UpdateFullScreenData()
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
    }

    bool FullScreenPass::OnWindowResize(const WindowResizeEvent& e)
    {
        UpdateFullScreenData();

        return false;
    }
}
#include "renderer/renderer.hpp"
#include "core/servicelocator.hpp"
#include "renderer/gfxdevice.hpp"
#include "renderer/command_buffer.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "scene/scene.hpp"
#include "core/job_system.hpp"
#include "editor/editor.hpp"
#include "memory/memory_service.hpp"

#include "renderer/render_passes/depth_pass.hpp"
#include "renderer/render_passes/forward_pass.hpp"
#include "renderer/render_passes/geometry_pass.hpp"
#include "renderer/render_passes/transparency_pass.hpp"
#include "renderer/render_passes/fullscreen_pass.hpp"
#include "renderer/render_passes/lighting_pass.hpp"
#include "renderer/render_passes/shadow_pass.hpp"
#include "renderer/render_passes/ssao_pass.hpp"
#include "renderer/render_passes/ssr_pass.hpp"
#include "renderer/render_passes/frustum_culling_pass.hpp"
#include "renderer/render_passes/fxaa_pass.hpp"

namespace Raw::GFX
{
    f32 elapsedTime = 0.0f;
    IGFXDevice* device = nullptr;
    GlobalSceneData sceneData;
    RenderPassData data;

    struct Renderer::pImplRenderer
    {
        void Shutdown();

        GeometryPass* m_GeometryPass{ nullptr };
        TransparencyPass* m_TransparencyPass{ nullptr };
        FullScreenPass* m_FullScreenPass{ nullptr };
        LightingPass* m_LightingPass{ nullptr };
        ShadowPass* m_ShadowPass{ nullptr };
        SSAOPass* m_SSAOPass{ nullptr };
        SSRPass* m_SSRPass{ nullptr };
        FrustumCullingPass* m_FrustumCullingPass{ nullptr };
        FXAAPass* m_FXAAPass{ nullptr };

        BufferHandle m_SceneDataBuffer;
    };

    void Renderer::pImplRenderer::Shutdown()
    {
        RAW_DEALLOCATE(m_GeometryPass);
        RAW_DEALLOCATE(m_TransparencyPass);
        RAW_DEALLOCATE(m_FullScreenPass);
        RAW_DEALLOCATE(m_ShadowPass);
        RAW_DEALLOCATE(m_SSAOPass);
        RAW_DEALLOCATE(m_SSRPass);
        RAW_DEALLOCATE(m_LightingPass);
        RAW_DEALLOCATE(m_FXAAPass);
        RAW_DEALLOCATE(m_FrustumCullingPass);
    }

    void Renderer::Init()
    {
        device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);

        void* implData = RAW_ALLOCATE(sizeof(Renderer::pImplRenderer), alignof(Renderer::pImplRenderer));
        m_Impl = new (implData) pImplRenderer();

        void* geomData = RAW_ALLOCATE(sizeof(GeometryPass), alignof(GeometryPass));
        m_Impl->m_GeometryPass = new (geomData) GeometryPass();
        m_Impl->m_GeometryPass->Init(device);
        
        void* transparencyData = RAW_ALLOCATE(sizeof(TransparencyPass), alignof(TransparencyPass));
        m_Impl->m_TransparencyPass = new (transparencyData) TransparencyPass();
        m_Impl->m_TransparencyPass->Init(device);

        void* ssaoData = RAW_ALLOCATE(sizeof(SSAOPass), alignof(SSAOPass));
        m_Impl->m_SSAOPass = new (ssaoData) SSAOPass();
        m_Impl->m_SSAOPass->Init(device);

        void* ssrData = RAW_ALLOCATE(sizeof(SSRPass), alignof(SSRPass));
        m_Impl->m_SSRPass = new (ssrData) SSRPass();
        m_Impl->m_SSRPass->Init(device);

        void* lightingData = RAW_ALLOCATE(sizeof(LightingPass), alignof(LightingPass));
        m_Impl->m_LightingPass = new (lightingData) LightingPass();
        m_Impl->m_LightingPass->Init(device);
        m_Impl->m_LightingPass->UpdateLightingData();

        void* fsData = RAW_ALLOCATE(sizeof(FullScreenPass), alignof(FullScreenPass));
        m_Impl->m_FullScreenPass = new (fsData) FullScreenPass();
        m_Impl->m_FullScreenPass->Init(device);
        m_Impl->m_FullScreenPass->UpdateFullScreenData();

        void* shadowData = RAW_ALLOCATE(sizeof(ShadowPass), alignof(ShadowPass));
        m_Impl->m_ShadowPass = new (shadowData) ShadowPass();
        m_Impl->m_ShadowPass->Init(device);

        void* fcData = RAW_ALLOCATE(sizeof(FrustumCullingPass), alignof(FrustumCullingPass));
        m_Impl->m_FrustumCullingPass = new (fcData) FrustumCullingPass();
        m_Impl->m_FrustumCullingPass->Init(device);

        void* fxaaData = RAW_ALLOCATE(sizeof(FXAAPass), alignof(FXAAPass));
        m_Impl->m_FXAAPass = new (fxaaData) FXAAPass();
        m_Impl->m_FXAAPass->Init(device);

        BufferDesc sceneDataDesc;
        sceneDataDesc.bufferSize = sizeof(GlobalSceneData);
        sceneDataDesc.memoryType = EMemoryType::HOST_VISIBLE;
        sceneDataDesc.type = EBufferType::UNIFORM;

        GlobalSceneData initialData;
        initialData.lightProj[1][1] *= -1;

        BufferResource* sceneDataBuffer = (BufferResource*)BufferLoader::Instance()->CreateBuffer("Scene Data Buffer", sceneDataDesc);
        m_Impl->m_SceneDataBuffer = sceneDataBuffer->buffer;

        device->MapBuffer(m_Impl->m_SceneDataBuffer, &initialData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_Impl->m_SceneDataBuffer, EBufferMapType::SCENE);
    }

    void Renderer::Shutdown()
    {
        m_Impl->Shutdown();
        
        RAW_DEALLOCATE(m_Impl);
    }

    void Renderer::Render(Scene* scene, Camera& camera, f32 dt)
    {
        elapsedTime += dt;

        TextureResource* tex = (TextureResource*)TextureLoader::Instance()->Get(DIR_SHADOW_MAP);

        sceneData.view = camera.GetViewMatrix();
        sceneData.viewInv = glm::inverse(sceneData.view);
        sceneData.projection = camera.GetProjectionMatrix();
        sceneData.projInv = glm::inverse(sceneData.projection);
        sceneData.viewProj = sceneData.projection * sceneData.view;
        sceneData.cameraFrustum = camera.GetFrustum();
        if(tex) sceneData.shadowMapIndex = tex->handle.id;
        
        device->MapBuffer(m_Impl->m_SceneDataBuffer, &sceneData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_Impl->m_SceneDataBuffer, EBufferMapType::SCENE);

        scene->Update(device);

        device->BeginOverlay();
        Editor::Get()->Render(dt, *scene->GetSceneData(), sceneData, &data);
        
        device->BeginFrame();
        
        ICommandBuffer* cmd = device->GetCommandBuffer();
        
        cmd->BeginCommandBuffer();
        cmd->Clear(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
       
        m_Impl->m_FrustumCullingPass->Execute(device, cmd, scene->GetSceneData());
        m_Impl->m_GeometryPass->Execute(device, cmd, scene->GetSceneData());
        m_Impl->m_TransparencyPass->Execute(device, cmd, scene->GetSceneData());
        m_Impl->m_ShadowPass->Execute(device, cmd, scene->GetSceneData());
        
        if(data.enableAO) m_Impl->m_SSAOPass->Execute(device, cmd, nullptr);
        if(data.enableSSR) m_Impl->m_SSRPass->Execute(device, cmd, nullptr);
        m_Impl->m_LightingPass->Execute(device, cmd, scene->GetSceneData());
        if(data.enableFXAA) m_Impl->m_FXAAPass->Execute(device, cmd, nullptr);
        m_Impl->m_FullScreenPass->Execute(device, cmd, scene->GetSceneData());

        device->EndFrame();
    }
}
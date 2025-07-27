#include "renderer/renderer.hpp"
#include "core/servicelocator.hpp"
#include "renderer/gfxdevice.hpp"
#include "renderer/command_buffer.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include "scene/scene.hpp"
#include "core/job_system.hpp"
#include "editor/editor.hpp"

#include "renderer/render_passes/depth_pass.hpp"
#include "renderer/render_passes/forward_pass.hpp"
#include "renderer/render_passes/geometry_pass.hpp"
#include "renderer/render_passes/transparency_pass.hpp"
#include "renderer/render_passes/fullscreen_pass.hpp"
#include "renderer/render_passes/shadow_pass.hpp"
#include "renderer/render_passes/ssao_pass.hpp"
#include "renderer/render_passes/ssr_pass.hpp"
#include "renderer/render_passes/frustum_culling_pass.hpp"
#include "renderer/render_passes/fxaa_pass.hpp"

namespace Raw::GFX
{
    f32 elapsedTime = 0.0f;
    GlobalSceneData sceneData;
    RenderPassData data;

    void Renderer::Init()
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);

        m_GeometryPass = new GeometryPass();
        m_GeometryPass->Init(device);
        
        m_TransparencyPass = new TransparencyPass();
        m_TransparencyPass->Init(device);

        m_SSAOPass = new SSAOPass();
        m_SSAOPass->Init(device);

        m_SSRPass = new SSRPass();
        m_SSRPass->Init(device);

        m_FullScreenPass = new FullScreenPass();
        m_FullScreenPass->Init(device);
        m_FullScreenPass->UpdateFullScreenData();

        m_ShadowPass = new ShadowPass();
        m_ShadowPass->Init(device);

        m_FrustumCullingPass = new FrustumCullingPass();
        m_FrustumCullingPass->Init(device);

        m_FXAAPass = new FXAAPass();
        m_FXAAPass->Init(device);

        BufferDesc sceneDataDesc;
        sceneDataDesc.bufferSize = sizeof(GlobalSceneData);
        sceneDataDesc.memoryType = EMemoryType::HOST_VISIBLE;
        sceneDataDesc.type = EBufferType::UNIFORM;

        GlobalSceneData initialData;
        initialData.lightProj[1][1] *= -1;

        BufferResource* sceneDataBuffer = (BufferResource*)BufferLoader::Instance()->CreateBuffer("Scene Data Buffer", sceneDataDesc);
        m_SceneDataBuffer = sceneDataBuffer->buffer;

        device->MapBuffer(m_SceneDataBuffer, &initialData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_SceneDataBuffer, EBufferMapType::SCENE);
    }

    void Renderer::Shutdown()
    {
        delete m_GeometryPass;
        delete m_TransparencyPass;
        delete m_FullScreenPass;
        delete m_ShadowPass;
        delete m_SSAOPass;
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

        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        
        device->MapBuffer(m_SceneDataBuffer, &sceneData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_SceneDataBuffer, EBufferMapType::SCENE);

        scene->Update(device);

        device->BeginOverlay();
        Editor::Get()->Render(dt, *scene->GetSceneData(), sceneData, &data);
        
        device->BeginFrame();
        
        ICommandBuffer* cmd = device->GetCommandBuffer();
        
        cmd->BeginCommandBuffer();
        cmd->Clear(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
       
        m_FrustumCullingPass->Execute(device, cmd, scene->GetSceneData());
        m_GeometryPass->Execute(device, cmd, scene->GetSceneData());
        m_TransparencyPass->Execute(device, cmd, scene->GetSceneData());
        m_ShadowPass->Execute(device, cmd, scene->GetSceneData());
        
        if(data.enableAO) m_SSAOPass->Execute(device, cmd, nullptr);
        if(data.enableSSR) m_SSRPass->Execute(device, cmd, nullptr);
        m_FullScreenPass->Execute(device, cmd, scene->GetSceneData());
        if(data.enableFXAA) m_FXAAPass->Execute(device, cmd, nullptr);

        device->EndFrame();
    }
}
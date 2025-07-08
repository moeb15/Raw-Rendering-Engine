#include "renderer/renderer.hpp"
#include "core/servicelocator.hpp"
#include "renderer/gfxdevice.hpp"
#include "renderer/command_buffer.hpp"
#include "resources/buffer_loader.hpp"
#include "core/job_system.hpp"
#include "editor/editor.hpp"

#include "renderer/render_passes/depth_pass.hpp"
#include "renderer/render_passes/forward_pass.hpp"

namespace Raw::GFX
{
    f32 elapsedTime = 0.0f;
    GlobalSceneData sceneData;

    void Renderer::Init()
    {
        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);

        m_DepthPass = new DepthPass();
        m_DepthPass->Init(device);

        m_ForwardPass = new ForwardPass();
        m_ForwardPass->Init(device);
        
        BufferDesc sceneDataDesc;
        sceneDataDesc.bufferSize = sizeof(GlobalSceneData);
        sceneDataDesc.memoryType = EMemoryType::HOST_VISIBLE;
        sceneDataDesc.type = EBufferType::UNIFORM;

        GlobalSceneData initialData;
        initialData.lightProj[1][1] *= -1;

        BufferResource* sceneDataBuffer = (BufferResource*)BufferLoader::Instance()->CreateBuffer("Scene Data Buffer", sceneDataDesc);
        m_SceneDataBuffer = sceneDataBuffer->buffer;

        device->MapBuffer(m_SceneDataBuffer, &initialData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_SceneDataBuffer, true);
    }

    void Renderer::Shutdown()
    {
        delete m_DepthPass;
    }

    void Renderer::Render(SceneData* scene, Camera& camera, f32 dt)
    {
        elapsedTime += dt;

        sceneData.view = camera.GetViewMatrix();
        sceneData.projection = camera.GetProjectionMatrix();
        sceneData.viewProj = sceneData.projection * sceneData.view;

        IGFXDevice* device = (IGFXDevice*)ServiceLocator::Get()->GetService(IGFXDevice::k_ServiceName);
        
        device->MapBuffer(m_SceneDataBuffer, &sceneData, sizeof(GlobalSceneData));
        device->UnmapBuffer(m_SceneDataBuffer, true);
        
        device->BeginOverlay();
        Editor::Get()->Render(dt, *scene, sceneData);
        
        device->BeginFrame();
        
        ICommandBuffer* cmd = device->GetCommandBuffer();
        
        cmd->BeginCommandBuffer();
        cmd->Clear(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        m_DepthPass->Execute(device, cmd, scene);
        m_ForwardPass->Execute(device, cmd, scene);

        device->EndFrame();
    }
}
#pragma once

#include "renderer/renderer_data.hpp"
#include "renderer/camera.hpp"
#include "resources/resource_manager.hpp"
#include "resources/texture_loader.hpp"

namespace Raw
{
    class Scene;
}

namespace Raw::GFX
{
    class DepthPass;
    class ForwardPass;

    class Renderer
    {
    public:
        void Init();
        void Shutdown();
        void Render(Scene* scene, Camera& camera, f32 dt);

    private:
        DepthPass* m_DepthPass;
        ForwardPass* m_ForwardPass;

        BufferHandle m_SceneDataBuffer;
    };
}
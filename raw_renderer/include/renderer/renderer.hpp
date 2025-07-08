#pragma once

#include "renderer/renderer_data.hpp"
#include "renderer/camera.hpp"
#include "resources/resource_manager.hpp"
#include "resources/texture_loader.hpp"

namespace Raw::GFX
{
    class DepthPass;

    class Renderer
    {
    public:
        void Init();
        void Shutdown();
        void Render(SceneData* scene, Camera& camera, f32 dt);

    private:
        DepthPass* m_DepthPass;

        BufferHandle m_SceneDataBuffer;
    };
}
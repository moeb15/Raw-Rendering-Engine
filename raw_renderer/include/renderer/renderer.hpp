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
    struct RenderPassData
    {
        bool enableAO{ true };
        bool enableSSR{ true };
        bool enableFXAA{ false };
    };

    class Renderer
    {
    public:
        void Init();
        void Shutdown();
        void Render(Scene* scene, Camera& camera, f32 dt);

    private:
        struct pImplRenderer;
        pImplRenderer* m_Impl;

    };
}
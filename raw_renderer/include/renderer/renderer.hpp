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
    class GeometryPass;
    class TransparencyPass;
    class FullScreenPass;
    class ShadowPass;
    class SSAOPass;
    class SSRPass;

    class Renderer
    {
    public:
        void Init();
        void Shutdown();
        void Render(Scene* scene, Camera& camera, f32 dt);

    private:
        DepthPass* m_DepthPass;
        ForwardPass* m_ForwardPass;
        GeometryPass* m_GeometryPass;
        TransparencyPass* m_TransparencyPass;
        FullScreenPass* m_FullScreenPass;
        ShadowPass* m_ShadowPass;
        SSAOPass* m_SSAOPass;
        SSRPass* m_SSRPass;

        BufferHandle m_SceneDataBuffer;
    };
}
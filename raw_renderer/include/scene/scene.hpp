#pragma once

#include "core/defines.hpp"
#include "renderer/renderer_data.hpp"
#include "renderer/gfxdevice.hpp"
#include "scene/scene_graph.hpp"
#include "memory/smart_pointers.hpp"
#include "containers/vector.hpp"
#include <string>

namespace Raw
{
    class Scene
    {
    public:
        DISABLE_COPY(Scene);

        Scene() {}
        ~Scene() {}

        void Init(std::string& filePath, GFX::IGFXDevice* device);
        void Shutdown();
        void Update(GFX::IGFXDevice* device);

        GFX::SceneData* GetSceneData() const { return m_SceneData.get(); }
        GFX::PointLight* GetPointLights() const { return m_PointLights.data(); }
        GFX::BufferHandle GetSceneMaterials() const { return m_MaterialDataBuffer; }

    private:
        std::string m_Filepath{ "" };
        rstd::unique_ptr<GFX::SceneData> m_SceneData{ nullptr };
        GFX::BufferHandle m_MaterialDataBuffer;
        rstd::vector<GFX::PointLight> m_PointLights;
        GFX::BufferHandle m_PointLightBuffer;
    };
}
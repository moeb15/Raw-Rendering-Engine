#pragma once

#include "core/defines.hpp"
#include "renderer/renderer_data.hpp"
#include "scene/scene_graph.hpp"
#include <string>
#include <memory>

namespace Raw
{
    class Scene
    {
    public:
        DISABLE_COPY(Scene);

        Scene() {}
        ~Scene() {}

        void Init(std::string& filePath);
        void Shutdown();

        GFX::SceneData* GetSceneData() const { return m_SceneData.get(); }

    private:
        std::string m_Filepath{ "" };
        std::unique_ptr<GFX::SceneData> m_SceneData{ nullptr };
    };
}
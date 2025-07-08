#include "scene/scene.hpp"
#include "utility/gltf.hpp"

namespace Raw
{
    void Scene::Init(std::string& filePath)
    {
        if(m_SceneData) m_SceneData.reset();

        m_SceneData = std::make_unique<GFX::SceneData>();
        Utils::LoadGLTF(filePath, *m_SceneData.get());
    }
    
    void Scene::Shutdown()
    {
        m_SceneData.reset();
    }
}
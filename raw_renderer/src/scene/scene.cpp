#include "scene/scene.hpp"
#include "utility/gltf.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"

namespace Raw
{
    void Scene::Init(std::string& filePath)
    {
        if(m_SceneData)
        {
            BufferLoader::Instance()->Unload(m_SceneData->vertexBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->indexBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->indirectBufferId);

            for(u64 i = 0; i < m_SceneData->imageIds.size(); i++)
            {
                TextureLoader::Instance()->Unload(m_SceneData->imageIds[i]);
            }

            m_SceneData.reset();
        }

        m_SceneData = std::make_unique<GFX::SceneData>();
        Utils::LoadGLTF(filePath, *m_SceneData.get());
    }
    
    void Scene::Shutdown()
    {
        m_SceneData.reset();
    }
}
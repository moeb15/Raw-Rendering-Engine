#include "scene/scene.hpp"
#include "utility/gltf.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"

namespace Raw
{
    namespace
    {
        void ProcessMaterialData(std::vector<GFX::PBRMaterialData>& matData, GFX::SceneData* sceneData)
        {
            matData.resize(GFX::MAX_MATERIALS);

            GFX::TextureHandle errorTexture;
            GFX::TextureHandle defaultTexture;
            GFX::TextureHandle defaultEmissive;

            TextureResource* tex = (TextureResource*)TextureLoader::Instance()->Get(ERROR_TEXTURE);
            errorTexture = tex->handle;

            tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_TEXTURE);
            defaultTexture = tex->handle;

            tex = (TextureResource*)TextureLoader::Instance()->Get(DEFAULT_EMISSIVE);
            defaultEmissive = tex->handle;

            for(u32 i = 0; i < matData.size(); i++)
            {
                matData[i].diffuse = errorTexture.id;
                matData[i].roughness = defaultTexture.id;
                matData[i].occlusion = defaultTexture.id;
                matData[i].normal = defaultTexture.id;
                matData[i].emissive = defaultEmissive.id;
            }

            for(u32 i = 0; i < sceneData->materials.size(); i++)
            {
                if(i >= matData.size()) break;
                
                GFX::PBRMaterialData material = sceneData->materials[i];
            
                u32 diffuse = errorTexture.id;
                u32 normal = defaultTexture.id;
                u32 roughness = defaultTexture.id;
                u32 occlusion = defaultTexture.id;
                u32 emissive = defaultEmissive.id;

                if(material.diffuse != -1)      diffuse = sceneData->images[sceneData->textures[material.diffuse]];
                if(material.normal != -1)       normal = sceneData->images[sceneData->textures[material.normal]];
                if(material.roughness != -1)    roughness = sceneData->images[sceneData->textures[material.roughness]];
                if(material.occlusion != -1)    occlusion = sceneData->images[sceneData->textures[material.occlusion]];
                if(material.emissive != -1)     emissive = sceneData->images[sceneData->textures[material.emissive]];

                material.diffuse = diffuse;
                material.roughness = roughness;
                material.occlusion = occlusion;
                material.normal = normal;
                material.emissive = emissive;

                matData[i] = material;
            }
        }
    }

    void Scene::Init(std::string& filePath, GFX::IGFXDevice* device)
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

        std::vector<GFX::PBRMaterialData> matData;
        ProcessMaterialData(matData, m_SceneData.get());

        u64 materialSize = sizeof(GFX::PBRMaterialData);

        GFX::BufferDesc materialDataDesc;
        materialDataDesc.bufferSize = GFX::MAX_MATERIALS * materialSize;
        materialDataDesc.memoryType = GFX::EMemoryType::HOST_VISIBLE;
        materialDataDesc.type = GFX::EBufferType::UNIFORM;

        m_MaterialDataBuffer = device->CreateBuffer(materialDataDesc);
        device->MapBuffer(m_MaterialDataBuffer, matData.data(), matData.size() * materialSize);
        device->UnmapBuffer(m_MaterialDataBuffer, GFX::EBufferMapType::MATERIAL);
    }
    
    void Scene::Update(GFX::IGFXDevice* device)
    {
        std::vector<GFX::PBRMaterialData> matData;
        ProcessMaterialData(matData, m_SceneData.get());
        
        u64 materialSize = sizeof(GFX::PBRMaterialData);
        device->MapBuffer(m_MaterialDataBuffer, matData.data(), matData.size() * materialSize);
        device->UnmapBuffer(m_MaterialDataBuffer, GFX::EBufferMapType::MATERIAL);
    }

    void Scene::Shutdown()
    {
        m_SceneData.reset();
    }
}
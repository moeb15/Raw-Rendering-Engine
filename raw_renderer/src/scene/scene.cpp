#include "scene/scene.hpp"
#include "utility/gltf.hpp"
#include "resources/buffer_loader.hpp"
#include "resources/texture_loader.hpp"
#include <cstdlib>
#include <cmath>
#include "core/timer.hpp"

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
        u32 curTime = (u32)Timer::Get()->Now();
        srand(curTime);

        if(m_SceneData)
        {
            BufferLoader::Instance()->Unload(m_SceneData->vertexBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->indexBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->indirectBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->culledIndirectBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->meshDrawsBufferId);
            BufferLoader::Instance()->Unload(m_SceneData->meshDrawsBufferId);

            for(u64 i = 0; i < m_SceneData->imageIds.size(); i++)
            {
                TextureLoader::Instance()->Unload(m_SceneData->imageIds[i]);
            }

            m_SceneData.reset();
        }

        m_SceneData = rstd::make_unique<GFX::SceneData>();
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

        m_PointLights.resize(MAX_LIGHT_COUNT);
        for(u32 i = 0; i < m_PointLights.size(); i++)
        {
            f32 x = (f32)((rand() % 10) * pow(-1, curTime + 1)); 
            f32 y = (f32)((rand() % 10) * pow(-1, curTime - 1)); 
            f32 z = (f32)((rand() % 10) * pow(-1, curTime));

            f32 r = (f32)rand() / (f32)(RAND_MAX);
            f32 g = (f32)rand() / (f32)(RAND_MAX);
            f32 b = (f32)rand() / (f32)(RAND_MAX);

            m_PointLights[i].position.x = x;
            m_PointLights[i].position.y = y;
            m_PointLights[i].position.z = z;

            m_PointLights[i].direction.x = 0.0f;
            m_PointLights[i].direction.y = -1.0f;
            m_PointLights[i].direction.z = 0.0f;

            m_PointLights[i].color.r = r;
            m_PointLights[i].color.g = g;
            m_PointLights[i].color.b = b;
            m_PointLights[i].color.a = 1.0f;

            m_PointLights[i].intensity = 1.f;
            m_PointLights[i].radius = 10.f;
        }

        u64 lightSize = sizeof(GFX::PointLight);
        GFX::BufferDesc lightBufferDesc;
        lightBufferDesc.bufferSize = MAX_LIGHT_COUNT * lightSize;
        lightBufferDesc.memoryType = GFX::EMemoryType::HOST_VISIBLE;
        lightBufferDesc.type = GFX::EBufferType::UNIFORM;

        m_PointLightBuffer = device->CreateBuffer(materialDataDesc);
        device->MapBuffer(m_PointLightBuffer, m_PointLights.data(), m_PointLights.size() * lightSize);
        device->UnmapBuffer(m_PointLightBuffer, GFX::EBufferMapType::POINTLIGHT);
    }
    
    void Scene::Update(GFX::IGFXDevice* device)
    {
        std::vector<GFX::PBRMaterialData> matData;
        ProcessMaterialData(matData, m_SceneData.get());
        
        u64 materialSize = sizeof(GFX::PBRMaterialData);
        device->MapBuffer(m_MaterialDataBuffer, matData.data(), matData.size() * materialSize);
        device->UnmapBuffer(m_MaterialDataBuffer, GFX::EBufferMapType::MATERIAL);

        u64 lightSize = sizeof(GFX::PointLight);
        device->MapBuffer(m_PointLightBuffer, m_PointLights.data(), m_PointLights.size() * lightSize);
        device->UnmapBuffer(m_PointLightBuffer, GFX::EBufferMapType::POINTLIGHT);
    }

    void Scene::Shutdown()
    {
        m_SceneData.reset();
        m_PointLights.shutdown();
    }
}
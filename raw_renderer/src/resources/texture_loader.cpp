#include "resources/texture_loader.hpp"
#include "renderer/gfxdevice.hpp"
#include "core/servicelocator.hpp"
#include "core/logger.hpp"
#include "utility/hash.hpp"
#include "stb_image.h"

namespace Raw
{
    static TextureLoader s_TextureLoader;

    TextureLoader* TextureLoader::Instance()
    {
        return &s_TextureLoader;
    }

    void TextureLoader::Init()
    {
        std::string errDir = RAW_RESOURCES_DIR + "/Error.png";
        std::string defaultTexDir = RAW_RESOURCES_DIR + "/Default.png";
        std::string defaultEmissiveDir = RAW_RESOURCES_DIR + "/DefaultEmissive.png";

        TextureLoader::Instance()->CreateFromFile(ERROR_TEXTURE, errDir.c_str());
        TextureLoader::Instance()->CreateFromFile(DEFAULT_TEXTURE, defaultTexDir.c_str());
        TextureLoader::Instance()->CreateFromFile(DEFAULT_EMISSIVE, defaultEmissiveDir.c_str());
    }   

    void TextureLoader::Shutdown()
    {
        RAW_INFO("TextureLoader shutting down...");
        for(auto& kvPair : m_TextureMap)
        {
            kvPair.second.reset();
        }
        m_TextureMap.clear();
    }

    Resource* TextureLoader::Get(cstring name)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_TextureMap.find(hashedName) != m_TextureMap.end())
        {
            m_TextureMap[hashedName]->AddRef();
            return m_TextureMap[hashedName].get();
        }
        
        RAW_DEBUG("TextureLoader could not find texture '%s'", name);
        u64 errHash = Utils::HashCString(ERROR_TEXTURE);
        return m_TextureMap[errHash].get();
    }

    Resource* TextureLoader::Get(u64 hashedName)
    {
        if(m_TextureMap.find(hashedName) != m_TextureMap.end())
        {
            m_TextureMap[hashedName]->AddRef();
            return m_TextureMap[hashedName].get();
        }
        
        RAW_DEBUG("TextureLoader could not find texture id: %llu", hashedName);
        u64 errHash = Utils::HashCString(ERROR_TEXTURE);
        return m_TextureMap[errHash].get();
    }

    void TextureLoader::Unload(cstring name)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_TextureMap.find(hashedName) != m_TextureMap.end())
        {
            m_TextureMap[hashedName]->RemoveRef();
            if(m_TextureMap[hashedName]->refs == 0)
            {
                GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
                TextureResource* res = m_TextureMap[hashedName].get();
                device->DestroyTexture(res->handle);

                m_TextureMap.erase(hashedName);
            }
        }
        RAW_DEBUG("TextureLoader could not unload texture '%s'", name);
    }

    void TextureLoader::Unload(u64 hashedName)
    {
        if(m_TextureMap.find(hashedName) != m_TextureMap.end())
        {
            m_TextureMap[hashedName]->RemoveRef();
            if(m_TextureMap[hashedName]->refs == 0)
            {
                GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
                TextureResource* res = m_TextureMap[hashedName].get();
                device->DestroyTexture(res->handle);

                m_TextureMap.erase(hashedName);
            }
        }
        RAW_DEBUG("TextureLoader could not unload texture ID '%llu'", hashedName);
    }

    Resource* TextureLoader::CreateFromFile(cstring name, cstring filename)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_TextureMap.find(hashedName) != m_TextureMap.end()) return m_TextureMap[hashedName].get();
        
        int w, h, numCh;
        u8* imageData = stbi_load(filename, &w, &h, &numCh, 4);
        if(!imageData)
        {
            RAW_ERROR("TextureLoader failed to load image '%s', file path '%s'", name, filename);
            return nullptr;
        }
        GFX::TextureDesc desc;
        desc.width = (u32)w;
        desc.height = (u32)h;
        desc.depth = 1;
        desc.format = GFX::ETextureFormat::R8G8B8A8_UNORM;
        desc.isMipmapped = true;
        desc.isRenderTarget = false;
        desc.isStorageImage = false;
        desc.type = GFX::ETextureType::TEXTURE2D;
        std::unique_ptr<TextureResource> tex = std::make_unique<TextureResource>();
        tex->name = name;
        tex->textureId = hashedName;

        GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
        tex->handle = device->CreateTexture(desc, imageData);
        tex->AddRef();

        m_TextureMap.insert(std::pair<u64, std::unique_ptr<TextureResource>>(hashedName, std::move(tex)));

        return m_TextureMap[hashedName].get();
    }

    Resource* TextureLoader::CreateFromData(cstring name, const GFX::TextureDesc& desc, void* data)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_TextureMap.find(hashedName) != m_TextureMap.end()) return m_TextureMap[hashedName].get();

        std::unique_ptr<TextureResource> tex = std::make_unique<TextureResource>();
        tex->name = name;
        tex->textureId = hashedName;

        GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
        tex->handle = device->CreateTexture(desc, data);
        tex->AddRef();

        m_TextureMap.insert(std::pair<u64, std::unique_ptr<TextureResource>>(hashedName, std::move(tex)));
        return m_TextureMap[hashedName].get();
    }

    Resource* TextureLoader::CreateFromHandle(cstring name, const GFX::TextureHandle& texture)
    {
        RAW_ASSERT_MSG(texture.IsValid(), "Cannot create texture resource with invalid texture handle!");
        
        u64 hashedName = Utils::HashCString(name);
        if(m_TextureMap.find(hashedName) != m_TextureMap.end()) return m_TextureMap[hashedName].get();

        std::unique_ptr<TextureResource> tex = std::make_unique<TextureResource>();
        tex->name = name;
        tex->textureId = hashedName;
        tex->handle = texture;
        tex->AddRef();

        m_TextureMap.insert(std::pair<u64, std::unique_ptr<TextureResource>>(hashedName, std::move(tex)));
        return m_TextureMap[hashedName].get();
    }
}
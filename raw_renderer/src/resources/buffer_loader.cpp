#include "resources/buffer_loader.hpp"
#include "renderer/gfxdevice.hpp"
#include "core/servicelocator.hpp"
#include "core/logger.hpp"
#include "utility/hash.hpp"

namespace Raw
{
    static BufferLoader s_BufferLoader;
   
    BufferLoader* BufferLoader::Instance()
    {
        return &s_BufferLoader;
    }
    
    void BufferLoader::Init()
    {

    }

    void BufferLoader::Shutdown()
    {
        RAW_INFO("BufferLoader shutting down...");
        for(auto& kvPair : m_BufferMap)
        {
            kvPair.second.reset();
        }
        m_BufferMap.clear();
    }

    Resource* BufferLoader::CreateBuffer(cstring name, GFX::BufferDesc& desc, void* initialData)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_BufferMap.find(hashedName) != m_BufferMap.end())
        {
            RAW_DEBUG("Duplicate buffer id: %llu, name: %s", hashedName, name);
            return m_BufferMap[hashedName].get();
        }
        GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
        std::unique_ptr<BufferResource> res = std::make_unique<BufferResource>();

        res->bufferId = hashedName;
        if(initialData)
        {
            res->buffer = device->CreateBuffer(desc, initialData);
        }
        else
        {
            res->buffer = device->CreateBuffer(desc);
        }

        m_BufferMap.insert(std::pair<u64, std::unique_ptr<BufferResource>>(hashedName, std::move(res)));
        return m_BufferMap[hashedName].get();
    }

    Resource* BufferLoader::Get(cstring name)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_BufferMap.find(hashedName) != m_BufferMap.end())
        {
            m_BufferMap[hashedName]->AddRef();
            return m_BufferMap[hashedName].get();
        }
        
        RAW_DEBUG("BufferLoader could not find buffer '%s'", name);
        return nullptr;
    }

    Resource* BufferLoader::Get(u64 hashedName)
    {
        if(m_BufferMap.find(hashedName) != m_BufferMap.end())
        {
            m_BufferMap[hashedName]->AddRef();
            return m_BufferMap[hashedName].get();
        }
        
        RAW_DEBUG("BufferLoader could not find buffer id: %llu", hashedName);
        return nullptr;
    }

    void BufferLoader::Unload(cstring name)
    {
        u64 hashedName = Utils::HashCString(name);
        if(m_BufferMap.find(hashedName) != m_BufferMap.end())
        {
            m_BufferMap[hashedName]->RemoveRef();
            if(m_BufferMap[hashedName]->refs == 0)
            {
                GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
                BufferResource* res = m_BufferMap[hashedName].get();
                device->DestroyBuffer(res->buffer);

                m_BufferMap.erase(hashedName);
            }
        }
    }

    void BufferLoader::Unload(u64 hashedName)
    {
        if(m_BufferMap.find(hashedName) != m_BufferMap.end())
        {
            m_BufferMap[hashedName]->RemoveRef();
            if(m_BufferMap[hashedName]->refs == 0)
            {
                GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
                BufferResource* res = m_BufferMap[hashedName].get();
                device->DestroyBuffer(res->buffer);

                m_BufferMap.erase(hashedName);
            }
        }
    }
}
#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include <unordered_map>
#include <string>

namespace Raw
{
    static const std::string RAW_RESOURCES_DIR{ ASSETS_DIR };

    struct Resource
    {
        u64 refs{ 0 };
        cstring name{ nullptr };

        void AddRef() { refs++; };
        void RemoveRef() { RAW_ASSERT(refs > 0); refs--; }
    };

    class IResourceLoader
    {
    public:
        virtual void Init() {}
        virtual void Shutdown() {}
        virtual Resource* Get(cstring name) = 0;
        virtual Resource* Get(u64 hashedName) = 0;
        virtual void Unload(cstring name) = 0;
        virtual void Unload(u64 hashedName) = 0;
        virtual void Remove(cstring name) = 0;
        virtual void Remove(u64 hashedName) = 0;
        virtual Resource* CreateFromFile(cstring name, cstring filename) { return nullptr; }
    };

    class ResourceManager
    {
    public:
        DISABLE_COPY(ResourceManager);

        static ResourceManager* Get();

        ResourceManager() {}
        ~ResourceManager() {}

        void Init() {}
        void Shutdown();

        template <typename T>
        T* LoadResource(cstring name, cstring filename = nullptr);

        template <typename T>
        T* GetResource(cstring name);

        void SetLoader(cstring resourceType, IResourceLoader* loader)
        {
            if(m_Loaders.find(resourceType) == m_Loaders.end())
            {
                m_Loaders[resourceType] = loader;
            }
        }

        IResourceLoader* GetLoader(cstring resourceType)
        {
            if(m_Loaders.find(resourceType) != m_Loaders.end())
            {
                return m_Loaders[resourceType];
            }
            return nullptr;
        }

    private:
        std::unordered_map<cstring, IResourceLoader*> m_Loaders;

    };

    template <typename T>
    RAW_INLINE T* ResourceManager::GetResource(cstring name)
    {
        IResourceLoader* loader = m_Loaders.find(T::k_ResourceType);
        if(loader) return (T*)loader->Get(name); 
        
        return nullptr;
    }

    template <typename T>
    RAW_INLINE T* ResourceManager::LoadResource(cstring name, cstring filename)
    {
        IResourceLoader* loader = m_Loaders.find(T::k_ResourceType);
        if(loader)
        {
            // check if cached
            T* resource = (T*)loader->Get(name);
            if(resource) return resource;

            return loader->CreateFromFile(name, filename);
        }
        return nullptr;
    }
}
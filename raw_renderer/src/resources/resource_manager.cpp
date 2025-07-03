#include "resources/resource_manager.hpp"

namespace Raw
{
    static ResourceManager s_ResourceManager;

    ResourceManager* ResourceManager::Get()
    {
        return &s_ResourceManager;
    }

    void ResourceManager::Shutdown()
    {
        for(auto& kvPair : m_Loaders)
        {
            kvPair.second->Shutdown();
        }
    }
}
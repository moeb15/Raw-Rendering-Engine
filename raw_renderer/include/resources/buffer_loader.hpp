#pragma once

#include "renderer/renderer_data.hpp"
#include "resources/resource_manager.hpp"
#include "resources/resource_pool.hpp"
#include "memory/smart_pointers.hpp"
#include <unordered_map>

namespace Raw
{
    struct BufferResource : public Resource
    {   
        GFX::BufferHandle buffer;
        u64 bufferId;

        static constexpr cstring k_ResourceType = "buffer_resource";
    };

    class BufferLoader : public IResourceLoader
    {
    public:
        static BufferLoader* Instance();

        virtual void Init() override;
        virtual void Shutdown() override;
        Resource* CreateBuffer(cstring name, GFX::BufferDesc& desc, void* initialData = nullptr);
        virtual Resource* Get(cstring name) override;
        virtual Resource* Get(u64 hashedName) override;
        virtual void Unload(cstring name) override;
        virtual void Unload(u64 hashedName) override;
        
        virtual void Remove(cstring name) override {}
        virtual void Remove(u64 hashedName) override {}

    private:
        std::unordered_map<u64, rstd::unique_ptr<BufferResource>> m_BufferMap;

    };
}
#pragma once

#include "renderer/gpu_resources.hpp"
#include "resources/resource_manager.hpp"
#include <memory>
#include <unordered_map>

namespace Raw
{
#define DEFAULT_TEXTURE "default"
#define DEFAULT_EMISSIVE "defaultEmissive"
#define ERROR_TEXTURE "error"

    struct TextureResource : public Resource
    {
        GFX::TextureHandle handle;
        u64 textureId;

        static constexpr cstring k_ResourceType = "texture_resource";
    };

    class TextureLoader : public IResourceLoader
    {
    public:
        static TextureLoader* Instance();

        virtual void Init() override;
        virtual void Shutdown() override;
        virtual Resource* Get(cstring name) override;
        virtual Resource* Get(u64 hashedName) override;
        virtual void Unload(cstring name) override;
        virtual void Unload(u64 hashedName) override;
        virtual Resource* CreateFromFile(cstring name, cstring filename) override;
        Resource* CreateFromData(cstring name, const GFX::TextureDesc& desc, void* data);
        Resource* CreateFromHandle(cstring name, const GFX::TextureHandle& texture);
        
    private:
        std::unordered_map<u64, std::unique_ptr<TextureResource>> m_TextureMap;

    };
}
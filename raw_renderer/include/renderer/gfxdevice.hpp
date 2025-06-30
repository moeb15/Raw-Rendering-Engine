#pragma once

#include "core/service.hpp"
#include "renderer/renderer_enums.hpp"
#include "renderer/gpu_resources.hpp"
#include <string>
#include <utility>

#define MAX_SWAPCHAIN_IMAGES 3

namespace Raw::GFX
{
    struct DeviceConfig
    {
        EDeviceBackend rendererAPI;
        std::string name;
        u32 width;
        u32 height;
        u32 maxWidth;
        u32 maxHeight;
        void* windowHandle;
    };

    class ICommandBuffer;

    class IGFXDevice : public IService
    {
    public:
        virtual void InitializeDevice(DeviceConfig& config) = 0;
        virtual void Shutdown() override {}
        
        virtual void InitializeEditor() = 0;
        virtual void BeginOverlay() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual ICommandBuffer* GetCommandBuffer(bool begin = false) = 0;
        virtual ICommandBuffer* GetSecondaryCommandBuffer() = 0;
        virtual void MapBuffer(const BufferHandle& handle, void* data, u64 dataSize) = 0;
        virtual void UnmapBuffer(const BufferHandle& handle, bool isSceneData = false) = 0;
        virtual void MapTexture(const TextureHandle& handle, bool isBindless = true) = 0;
        virtual TextureHandle& GetDrawImageHandle() = 0;
        virtual TextureHandle& GetDepthBufferHandle() = 0;
        virtual void SubmitCommandBuffer(ICommandBuffer* cmd) = 0;
        virtual std::pair<u32, u32> GetBackBufferSize() = 0;

        [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDesc& desc, bool isDepth = false) = 0;
        [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDesc& desc, void* initialData) = 0;
        [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData) = 0;
        [[nodiscard]] virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
        [[nodiscard]] virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
        [[nodiscard]] virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
        
        virtual void DestroyTexture(TextureHandle& handle) = 0;
        virtual void DestroyBuffer(BufferHandle& handle) = 0;
        virtual void DestroySampler(SamplerHandle& handle) = 0;
        virtual void DestroyComputePipeline(ComputePipelineHandle& handle) = 0;
        virtual void DestroyGraphicsPipeline(GraphicsPipelineHandle& handle) = 0;

        virtual void DestroyTextureInstant(TextureHandle& handle) = 0;
        virtual void DestroyBufferInstant(BufferHandle& handle) = 0;
        virtual void DestroySamplerInstant(SamplerHandle& handle) = 0;
        virtual void DestroyComputePipelineInstant(ComputePipelineHandle& handle) = 0;
        virtual void DestroyGraphicsPipelineInstant(GraphicsPipelineHandle& handle) = 0;

        static constexpr cstring k_ServiceName = "raw_gfxdevice_service";
    };
}
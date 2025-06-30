#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include "events/event_handler.hpp"
#include "events/core_events.hpp"
#include "renderer/gfxdevice.hpp"
#include "renderer/command_buffer.hpp"

#if defined(RAW_PLATFORM_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Raw::GFX
{
#ifndef VK_CHECK
    #define VK_CHECK(res) RAW_ASSERT_MSG(res == VK_SUCCESS, "Vulkan assert code %i", res)
#endif

#ifndef VULKAN_MAX_BINDLESS_RESOURCES
    #define VULKAN_MAX_BINDLESS_RESOURCES 50000
#endif

#ifndef VULKAN_DEFAULT_RESOURCE_AMOUNT
    #define VULKAN_DEFAULT_RESOURCE_AMOUNT 12
#endif

#ifndef VULKAN_IMAGE_SAMPLER_BINDING
    #define VULKAN_IMAGE_SAMPLER_BINDING 0
#endif

#ifndef VULKAN_SAMPLED_IMAGE_BINDING
    #define VULKAN_SAMPLED_IMAGE_BINDING 1
#endif

#ifndef VULKAN_SAMPLER_BINDING
    #define VULKAN_SAMPLER_BINDING 2
#endif

#ifndef VULKAN_STORAGE_IMAGE_BINDING
    #define VULKAN_STORAGE_IMAGE_BINDING 3
#endif

#ifndef VULKAN_UBO_BINDING
    #define VULKAN_UBO_BINDING 4
#endif

#ifndef VULKAN_SSBO_BINDING
    #define VULKAN_SSBO_BINDING 5
#endif

    struct VulkanTexture;
    struct VulkanBuffer;
    struct VulkanPipeline;
    class VulkanCommandBuffer;
    class VulkanCommandBufferManager;

    class VulkanGFXDevice final : public IGFXDevice
    {
        friend class VulkanCommandBuffer;
        friend class VulkanCommandBufferManager;
    public:
        DISABLE_COPY(VulkanGFXDevice);
        RAW_DECLARE_SERVICE(VulkanGFXDevice);

        VulkanGFXDevice() {}
        ~VulkanGFXDevice() {}

        virtual void InitializeDevice(DeviceConfig& config) override;
        virtual void Shutdown() override;
        
        virtual void InitializeEditor() override;
        virtual void BeginOverlay() override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

        RAW_INLINE VkDevice GetDevice() { return m_LogicalDevice; }
        RAW_INLINE VmaAllocator GetAllocator() { return m_VmaAllocator; }
        RAW_INLINE VkAllocationCallbacks* GetAllocationCallbacks() { return m_AllocCallbacks; }
        VkImage GetDrawImage();
        VkExtent2D GetDrawExtent();
        VkImage GetDepthImage();

        virtual ICommandBuffer* GetCommandBuffer(bool begin = false) override;
        virtual ICommandBuffer* GetSecondaryCommandBuffer() override;
        virtual void MapBuffer(const BufferHandle& handle, void* data, u64 dataSize) override;
        virtual void UnmapBuffer(const BufferHandle& handle, bool isSceneData = false) override;
        virtual void MapTexture(const TextureHandle& handle, bool isBindless = true) override;
        virtual TextureHandle& GetDrawImageHandle() override;
        virtual TextureHandle& GetDepthBufferHandle() override;
        virtual void SubmitCommandBuffer(ICommandBuffer* cmd) override;
        virtual std::pair<u32, u32> GetBackBufferSize() override;

        // resource creation
        [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDesc& desc, bool isDepth = false) override;
        [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDesc& desc, void* initialData) override;
        [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc) override;
        [[nodsicard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData) override;
        [[nodiscard]] virtual SamplerHandle CreateSampler(const SamplerDesc& desc) override;
        [[nodiscard]] virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) override;
        [[nodiscard]] virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;

        // resource destruction
        virtual void DestroyTexture(TextureHandle& handle) override;
        virtual void DestroyBuffer(BufferHandle& handle) override;
        virtual void DestroySampler(SamplerHandle& handle) override;
        virtual void DestroyComputePipeline(ComputePipelineHandle& handle) override;
        virtual void DestroyGraphicsPipeline(GraphicsPipelineHandle& handle) override;

        virtual void DestroyTextureInstant(TextureHandle& handle) override;
        virtual void DestroyBufferInstant(BufferHandle& handle) override;
        virtual void DestroySamplerInstant(SamplerHandle& handle) override;
        virtual void DestroyComputePipelineInstant(ComputePipelineHandle& handle) override;
        virtual void DestroyGraphicsPipelineInstant(GraphicsPipelineHandle& handle) override;

        
        void SetResourceName(VkObjectType type, u64 vulkanHandle, cstring name);
        
    private:
        VulkanTexture* GetTexture(const TextureHandle& handle);
        VulkanBuffer* GetBuffer(const BufferHandle& handle);
        VkSampler* GetSampler(const SamplerHandle& handle);
        VulkanPipeline* GetComputePipeline(const ComputePipelineHandle& handle);
        VulkanPipeline* GetGraphicsPipeline(const GraphicsPipelineHandle& handle);

        RAW_INLINE VkQueue GetGFXQueue() { return m_GFXQueue; }
        RAW_INLINE VkQueue GetTransferQueue() { return m_TransferQueue; }

        RAW_INLINE u32 GetGFXQueueFamily() { return m_GFXQueueFamily; }
        RAW_INLINE u32 GetTransferQueueFamily() { return m_TransferQueueFamily; }
        
    private:
        bool GetFamilyQueue(VkPhysicalDevice gpu);
        void CreateSwapchain();
        void DestroySwapchain();
        void RecreateSwapchain();

        void DestroySynchronizationPrimitives();

        void LoadShader(ShaderDesc& desc, VkShaderModule& outModule);
        void PushMarker(VkCommandBuffer cmd, cstring name);
        void PopMarker(VkCommandBuffer cmd);

        void RenderOverlay(VkCommandBuffer cmd, VkImageView curSwapchainView);

    private:
        VkAllocationCallbacks* m_AllocCallbacks;
        VkInstance m_Instance{ VK_NULL_HANDLE };
        VkPhysicalDevice m_GPU{ VK_NULL_HANDLE };
        VkPhysicalDeviceProperties m_GPUProperties{};
        VkDevice m_LogicalDevice{ VK_NULL_HANDLE };

        VkQueue m_GFXQueue{ VK_NULL_HANDLE };
        u32 m_GFXQueueFamily{ 0 };
        VkQueue m_TransferQueue{ VK_NULL_HANDLE };
        u32 m_TransferQueueFamily{ 0 };

        VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
        VkSurfaceFormatKHR m_SurfaceFormat{};
        VkPresentModeKHR m_PresentMode{};

        VkImage m_SwapchainImages[MAX_SWAPCHAIN_IMAGES]{};
        VkImageView m_SwapchainImageViews[MAX_SWAPCHAIN_IMAGES]{};
        VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
        VkExtent2D m_SwapchainExtent{};
        u32 m_SwapchainImageCount{ MAX_SWAPCHAIN_IMAGES };
        u32 m_SwapchainImageIndex{ 0 };
        u32 m_CurFrame{ 0 };

        VkSemaphore m_RenderCompleteSemaphore[MAX_SWAPCHAIN_IMAGES]{};
        VkSemaphore m_ImageAcquiredSemaphore[MAX_SWAPCHAIN_IMAGES]{};
        VkFence m_CmdBufferExecutedFence[MAX_SWAPCHAIN_IMAGES]{};

        VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };

        VmaAllocator m_VmaAllocator{ VK_NULL_HANDLE };

        bool m_DebugUtilsPresent{ false };
        bool m_WindowMinimized{ false };
        bool m_WindowResized{ false };

        // bindless pool/descriptor set layout and descriptor set
        VkDescriptorPool m_BindlessPool{ VK_NULL_HANDLE };
        VkDescriptorSetLayout m_BindlessLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_BindlessSet{ VK_NULL_HANDLE };

        // used for global scene data
        VkDescriptorPool m_SceneDataPool{ VK_NULL_HANDLE };
        VkDescriptorSetLayout m_SceneLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_SceneDataSet[MAX_SWAPCHAIN_IMAGES]{};

        // default descriptor pool and mesh descriptor set/layout
        VkDescriptorPool m_DefaultPool{ VK_NULL_HANDLE };

        // default samplers
        SamplerHandle m_LinearSampler;
        SamplerHandle m_NearestSampler;
        SamplerHandle m_ShadowSampler;

        // imgui descriptor pool
        VkDescriptorPool m_ImGuiPool{ VK_NULL_HANDLE };
        VkDescriptorSet m_ImGuiDrawImage{ VK_NULL_HANDLE };

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;

    };
}
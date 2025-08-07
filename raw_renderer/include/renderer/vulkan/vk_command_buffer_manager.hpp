#pragma once

#include "renderer/command_buffer_manager.hpp"
#include "renderer/vulkan/vk_descriptors.hpp"
#include "resources/deletion_queue.hpp"
#include <vector>
#include <atomic>
#include <memory>
#include <vulkan/vulkan.h>

namespace Raw::GFX
{
    class VulkanCommandBuffer;

    struct VulkanFrameData
    {
        std::vector<VulkanCommandBuffer> primaryCmdBuffers;
        std::vector<VulkanCommandBuffer> secondaryCmdBuffers;
    };  

    class VulkanCommandBufferManager : public ICommandBufferManager
    {
    public:
        virtual void Init(u32 numThreads) override;
        virtual void Shutdown() override;

        virtual void ResetBuffers(u32 frameIndex) override;

        virtual ICommandBuffer* GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin) override;
        virtual ICommandBuffer* GetSecondaryCommandBuffer(u32 frameIndex, u32 threadIndex) override;

        std::vector<VkCommandPool> vulkanCmdPools;
        std::vector<VulkanFrameData> frameBuffer;

        std::vector<DeletionQueue> frameDelQueue;
        std::vector<VulkanDescriptorWriter> bindlessSetUpdates;
        std::vector<VulkanDescriptorWriter> sceneDataUpdates;
        std::vector<VulkanDescriptorWriter> materialDataUpdates;
        std::vector<VulkanDescriptorWriter> lightDataUpdates;

        u32 numPoolsPerFrame{ 1 };
        u32 threadCount{ 1 };
        u32 numPools{ 1 };
        u32 numPrimaryBuffersPerPool{ 3 };
        u32 numSecondaryBuffersPerPool{ 2 };
    };
}
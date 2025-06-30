#include "renderer/vulkan/vk_command_buffer_manager.hpp"
#include "renderer/vulkan/vk_command_buffer.hpp"
#include "renderer/vulkan/vk_gfxdevice.hpp"
#include "core/logger.hpp"
#include <mutex>

namespace Raw::GFX
{
    void VulkanCommandBufferManager::Init(u32 numThreads)
    {
        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();

        u32 numFramesInFlight = VulkanGFXDevice::Get()->m_SwapchainImageCount;
        numPoolsPerFrame = numFramesInFlight;
        threadCount = numThreads;
        numPools = threadCount * numPoolsPerFrame;
        numPrimaryBuffersPerPool = numFramesInFlight;

        frameBuffer.resize(numPools);
        vulkanCmdPools.resize(numPools);
        VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolInfo.queueFamilyIndex = VulkanGFXDevice::Get()->GetGFXQueueFamily();

        for(u32 i = 0; i < numPools; i++)
        {
            VK_CHECK(vkCreateCommandPool(device, &poolInfo, allocCallbacks, &vulkanCmdPools[i]));
            
            frameBuffer[i].primaryCmdBuffers.resize(numPrimaryBuffersPerPool);
            frameBuffer[i].secondaryCmdBuffers.resize(numSecondaryBuffersPerPool);

            VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            for(u32 j = 0; j < numPrimaryBuffersPerPool; j++)
            {
                cmdAllocInfo.commandBufferCount = 1;
                cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdAllocInfo.commandPool = vulkanCmdPools[i];

                vkAllocateCommandBuffers(device, &cmdAllocInfo, &frameBuffer[i].primaryCmdBuffers[j].vulkanCmdBuffer);
            }

            for(u32 k = 0; k < numSecondaryBuffersPerPool; k++)
            {
                cmdAllocInfo.commandBufferCount = 1;
                cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                cmdAllocInfo.commandPool = vulkanCmdPools[i];

                vkAllocateCommandBuffers(device, &cmdAllocInfo, &frameBuffer[i].secondaryCmdBuffers[k].vulkanCmdBuffer);
            }
        }

        frameDelQueue.resize(numFramesInFlight);
        bindlessSetUpdates.resize(numFramesInFlight);
        sceneDataUpdates.resize(numFramesInFlight);
    }

    void VulkanCommandBufferManager::Shutdown()
    {
        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();

        RAW_INFO("VulkanCommandBufferManager shutting down...");
        for(u32 i = 0; i < numPools; i++)
        {
            vkDestroyCommandPool(device, vulkanCmdPools[i], allocCallbacks);

            frameBuffer[i].primaryCmdBuffers.clear();
            frameBuffer[i].secondaryCmdBuffers.clear();
        }

        for(u32 i = 0; i < frameDelQueue.size(); i++)
        {
            frameDelQueue[i].Flush();
            bindlessSetUpdates[i].Shutdown();
            sceneDataUpdates[i].Shutdown();
        }


        vulkanCmdPools.clear();
        frameBuffer.clear();

        frameDelQueue.clear();
        bindlessSetUpdates.clear();
        sceneDataUpdates.clear();
    }

    void VulkanCommandBufferManager::ResetBuffers(u32 frameIndex)
    {
        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        for(u32 i = 0; i < threadCount; i++)
        {
            u32 poolIndex = frameIndex * threadCount + i;
            vkResetCommandPool(device, vulkanCmdPools[poolIndex], 0);
            for(u32 j = 0; j < frameBuffer[poolIndex].primaryCmdBuffers.size(); j++)
            {
                frameBuffer[poolIndex].primaryCmdBuffers[j].Reset();
            }
        }
    }

    ICommandBuffer* VulkanCommandBufferManager::GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin)
    {
        u32 poolIndex = frameIndex * threadCount + threadIndex;
        VulkanFrameData& data = frameBuffer[poolIndex];

        for(u32 i = 0; i < data.primaryCmdBuffers.size(); i++)
        {
            VulkanCommandBuffer& cmd = data.primaryCmdBuffers[i];
            if(cmd.GetState() == ECommandBufferState::READY)
            {
                if(begin) cmd.BeginCommandBuffer();
                return &cmd;
            }
        }
        
        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        data.primaryCmdBuffers.emplace_back();
        VulkanCommandBuffer& newPrimaryBuffer = data.primaryCmdBuffers.back(); 
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = vulkanCmdPools[poolIndex];
        vkAllocateCommandBuffers(device, &allocInfo, &newPrimaryBuffer.vulkanCmdBuffer);
        
        VulkanCommandBuffer* cmd = &newPrimaryBuffer;

        if(begin) cmd->BeginCommandBuffer();
        return cmd;
    }

    ICommandBuffer* VulkanCommandBufferManager::GetSecondaryCommandBuffer(u32 frameIndex, u32 threadIndex)
    {
        u32 poolIndex = frameIndex * threadCount + threadIndex;

        VulkanFrameData& data = frameBuffer[poolIndex];

        for(u32 i = 0; i < data.secondaryCmdBuffers.size(); i++)
        {
            VulkanCommandBuffer& cmd = data.secondaryCmdBuffers[i];
            if(cmd.GetState() == ECommandBufferState::READY)
            {
                cmd.BeginSecondaryCommandBuffer();
                return &cmd;
            }
        }

        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        data.secondaryCmdBuffers.emplace_back();
        VulkanCommandBuffer& newSecondaryBuffer = data.secondaryCmdBuffers.back(); 
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandPool = vulkanCmdPools[poolIndex];
        vkAllocateCommandBuffers(device, &allocInfo, &newSecondaryBuffer.vulkanCmdBuffer);
        
        VulkanCommandBuffer* cmd = &newSecondaryBuffer;

        cmd->BeginSecondaryCommandBuffer();
        return cmd;
    }
}
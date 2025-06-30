#include "renderer/vulkan/vk_immediate_executor.hpp"
#include "renderer/vulkan/vk_gfxdevice.hpp"
#include "core/asserts.hpp"

namespace Raw::GFX
{
    void VulkanImmediateExecutor::Init(VkQueue queue, u32 queueFamily)
    {
        RAW_ASSERT_MSG(m_Init == false, "VulkanImmediateExecutor has already been initialized!");

        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();

        VkCommandPoolCreateInfo cmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = queueFamily;
        VK_CHECK(vkCreateCommandPool(device, &cmdPoolInfo, allocCallbacks, &m_ImmPool));
        
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };       
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = m_ImmPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &m_ImmCmd));

        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fenceInfo, allocCallbacks, &m_ImmFence));
        
        m_Queue = queue;
        m_Init = true;
    }
    
    void VulkanImmediateExecutor::Shutdown()
    {
        RAW_ASSERT_MSG(m_Init == true, "VulkanImmediateExecutor hasn't been initialized, cannot call Shutdown!");

        RAW_INFO("Destroying VulkanImmediateExecutor");
        vkDestroyCommandPool(VulkanGFXDevice::Get()->GetDevice(), m_ImmPool, VulkanGFXDevice::Get()->GetAllocationCallbacks());
        vkDestroyFence(VulkanGFXDevice::Get()->GetDevice(), m_ImmFence, VulkanGFXDevice::Get()->GetAllocationCallbacks());
    }
    
    void VulkanImmediateExecutor::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func) const
    {
        RAW_ASSERT_MSG(m_Init == true, "VulkanImmediateExecutor hasn't been initialized, cannot call ImmediateSubmit!");

        VkDevice device = VulkanGFXDevice::Get()->GetDevice();

        VK_CHECK(vkResetFences(device, 1, &m_ImmFence));
        VK_CHECK(vkResetCommandBuffer(m_ImmCmd, 0));

        VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(m_ImmCmd, &info));

        func(m_ImmCmd);

        VK_CHECK(vkEndCommandBuffer(m_ImmCmd));

        VkCommandBufferSubmitInfo cmdSubmitInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
        cmdSubmitInfo.commandBuffer = m_ImmCmd;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_ImmCmd;

        VK_CHECK(vkQueueSubmit(m_Queue, 1, &submitInfo, m_ImmFence));
        VK_CHECK(vkWaitForFences(device, 1, &m_ImmFence, VK_TRUE, U64_MAX));
    }
}
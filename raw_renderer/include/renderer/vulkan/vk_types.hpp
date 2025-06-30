#pragma once

#include "renderer/vulkan/vk_gfxdevice.hpp"

namespace Raw::GFX
{
    struct VulkanTexture
    {
        VkImage image{ VK_NULL_HANDLE };
        VkImageView srv{ VK_NULL_HANDLE };
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VkExtent3D imageExtent{};
        VkFormat imageFormat{ VK_FORMAT_UNDEFINED };
        VkImageLayout imageLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
        bool isRenderTarget{ false };
        bool isStorageImage{ false };
        bool isDepthTexture{ false };

        void Destroy()
        {
            vkDestroyImageView(VulkanGFXDevice::Get()->GetDevice(), srv, VulkanGFXDevice::Get()->GetAllocationCallbacks());
            vmaDestroyImage(VulkanGFXDevice::Get()->GetAllocator(), image, allocation);
        }
    };

    struct VulkanBuffer
    {
        VkBuffer buffer{ VK_NULL_HANDLE };
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocInfo{};
        VkDeviceAddress bufferAddress{};
        VkBufferUsageFlags usageFlags{};

        void Destroy()
        {
            vmaDestroyBuffer(VulkanGFXDevice::Get()->GetAllocator(), buffer, allocation);
            allocInfo = {};
        }
    };

    struct VulkanPipeline
    {
        VkPipeline pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
        VkDescriptorSetLayout dsLayout{ VK_NULL_HANDLE };
        VkDescriptorSet set{ VK_NULL_HANDLE };
        TextureHandle* imageAttachements{ nullptr };
        u32 numImageAttachments{ 0 };
        TextureHandle* depthAttachment{ nullptr };
        cstring pipelineName{ nullptr };

        void Destroy()
        {
            RAW_DEBUG("Destroying VulkanPipeline: %s.", pipelineName);
            if(dsLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(VulkanGFXDevice::Get()->GetDevice(), dsLayout, VulkanGFXDevice::Get()->GetAllocationCallbacks());
            vkDestroyPipelineLayout(VulkanGFXDevice::Get()->GetDevice(), pipelineLayout, VulkanGFXDevice::Get()->GetAllocationCallbacks());
            vkDestroyPipeline(VulkanGFXDevice::Get()->GetDevice(), pipeline, VulkanGFXDevice::Get()->GetAllocationCallbacks());
        }
    };
}
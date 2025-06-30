#pragma once

#include "renderer/vulkan/vk_gfxdevice.hpp"
#include <vector>
#include <deque>

namespace Raw::GFX
{
    class VulkanDescriptorLayoutBuilder
    {
    public:
        void Init() 
        {
            m_Bindings.clear();
        }
        void Shutdown() 
        { 
            Clear();
        }

        void Clear()
        {
           m_Bindings.clear();
        }

        void AddBinding(u32 binding, u32 count, VkDescriptorType type, VkShaderStageFlags stages);

        [[nodiscard]] VkDescriptorSetLayout Build(void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

    private:
        std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

    };

    class VulkanDescriptorWriter
    {
    public:
        void Init() {}
        void Shutdown() { Clear(); }
        void Clear() { m_DescriptorWrites.clear(); }

        void WriteImage(u32 binding, VkImageView srv, VkSampler sampler, VkImageLayout layout, VkDescriptorType type, u32 arrElem = 0);
        void WriteBuffer(u32 binding, VkBuffer buffer, u64 size, u64 offset, VkDescriptorType type);
        void WriteSampler(u32 binding, VkSampler sampler);

        void UpdateSet(VkDescriptorSet set);

    private:
        struct DescriptorData
        {
            VkDescriptorImageInfo imageInfo;
            VkDescriptorBufferInfo bufferInfo;
            VkDescriptorImageInfo samplerInfo;
            VkWriteDescriptorSet write;
        };

        std::deque<DescriptorData> m_DescriptorWrites;
        
    };
}
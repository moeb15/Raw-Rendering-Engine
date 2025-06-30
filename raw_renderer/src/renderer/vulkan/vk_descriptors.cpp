#include "renderer/vulkan/vk_descriptors.hpp"

namespace Raw::GFX
{
    void VulkanDescriptorLayoutBuilder::AddBinding(u32 binding, u32 count, VkDescriptorType type, VkShaderStageFlags stages)
    {
        VkDescriptorSetLayoutBinding bindingInfo = {};
        bindingInfo.binding = binding;
        bindingInfo.descriptorCount = count;
        bindingInfo.descriptorType = type;
        bindingInfo.stageFlags = stages;

        m_Bindings.push_back(bindingInfo);
    }

    VkDescriptorSetLayout VulkanDescriptorLayoutBuilder::Build(void* pNext, VkDescriptorSetLayoutCreateFlags flags)
    {
        VkDevice device = VulkanGFXDevice::Get()->GetDevice();
        VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();

        VkDescriptorSetLayoutCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        info.flags = flags;
        info.pNext = pNext;
        info.bindingCount = m_Bindings.size();
        info.pBindings = m_Bindings.data();

        VkDescriptorSetLayout layout{ VK_NULL_HANDLE };
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, allocCallbacks, &layout));

        return layout;
    }

    void VulkanDescriptorWriter::WriteImage(u32 binding, VkImageView srv, VkSampler sampler, VkImageLayout layout, VkDescriptorType type, u32 arrElem)
    {
        DescriptorData& data = m_DescriptorWrites.emplace_back();
        data.imageInfo = { sampler, srv, layout };

        data.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        data.write.dstBinding = binding;
        data.write.dstSet = VK_NULL_HANDLE;
        data.write.descriptorCount = 1;
        data.write.descriptorType = type;
        data.write.dstArrayElement = arrElem;
        data.write.pImageInfo = &data.imageInfo;
    }
    
    void VulkanDescriptorWriter::WriteBuffer(u32 binding, VkBuffer buffer, u64 size, u64 offset, VkDescriptorType type)
    {
        DescriptorData& data = m_DescriptorWrites.emplace_back();
        data.bufferInfo = { buffer, offset, size };

        data.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        data.write.dstBinding = binding;
        data.write.dstSet = VK_NULL_HANDLE;
        data.write.descriptorCount = 1;
        data.write.descriptorType = type;
        data.write.pBufferInfo = &data.bufferInfo;
    }

    void VulkanDescriptorWriter::WriteSampler(u32 binding, VkSampler sampler)
    {
        DescriptorData& data = m_DescriptorWrites.emplace_back();
        data.samplerInfo = { sampler };

        data.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        data.write.dstBinding = binding;
        data.write.dstSet = VK_NULL_HANDLE;
        data.write.descriptorCount = 1;
        data.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        data.write.pImageInfo = &data.samplerInfo;
    }

    void VulkanDescriptorWriter::UpdateSet(VkDescriptorSet set)
    {
        if(m_DescriptorWrites.size() > 0)
        {
            VkDevice device = VulkanGFXDevice::Get()->GetDevice();
            VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();
            std::vector<VkWriteDescriptorSet> writes;
            writes.reserve(m_DescriptorWrites.size());

            for(DescriptorData& data : m_DescriptorWrites)
            {
                data.write.dstSet = set;
                writes.push_back(data.write);
            }

            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
            writes.clear();
            m_DescriptorWrites.clear();
        }
    }
}
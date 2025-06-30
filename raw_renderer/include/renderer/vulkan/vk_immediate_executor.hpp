#pragma once

#include "core/defines.hpp"
#include <vulkan/vulkan.h>
#include <functional>

namespace Raw::GFX
{
    class VulkanImmediateExecutor
    {
    public:
        void Init(VkQueue queue, u32 queueFamily);
        void Shutdown();
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func) const;

    private:
        bool m_Init{ false };    
        VkCommandPool m_ImmPool{ VK_NULL_HANDLE };
        VkCommandBuffer m_ImmCmd{ VK_NULL_HANDLE };
        VkFence m_ImmFence{ VK_NULL_HANDLE };
        VkQueue m_Queue{ VK_NULL_HANDLE };
        
    };
}
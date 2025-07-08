#include "renderer/vulkan/vk_gfxdevice.hpp"
#include "renderer/vulkan/vk_command_buffer.hpp"
#include "renderer/vulkan/vk_command_buffer_manager.hpp"
#include "events/event_manager.hpp"
#include "renderer/vulkan/vk_utilities.hpp"
#include "renderer/vulkan/vk_types.hpp"
#include "renderer/vulkan/vk_immediate_executor.hpp"
#include "resources/deletion_queue.hpp"
#include "renderer/vulkan/vk_descriptors.hpp"
#include "resources/resource_pool.hpp"
#include "core/servicelocator.hpp"
#include "platform/window.hpp"
#include "core/job_system.hpp"
#include <mutex>
#include <fstream>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Raw::GFX
{
    namespace
    {
        VulkanGFXDevice s_VulkanGFXDevice;

        const char* s_RequestedExtensions[] = 
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
#if defined(_DEBUG)
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };

        const char* s_RequestedLayers[] = 
        {
#if defined(_DEBUG)
            "VK_LAYER_KHRONOS_validation",
#else
            "",
#endif
        };

        VKAPI_ATTR VkBool32 DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
        {
            switch(messageSeverity)
            {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    RAW_ERROR(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    RAW_WARN(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    RAW_INFO(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    RAW_TRACE(pCallbackData->pMessage);
                    break;
                default: break;
            }

            return VK_FALSE;
        }
        PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
        PFN_vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

        struct VulkanResourceCache
        {
            void Shutdown();

            ResourcePool<VulkanTexture> textures;
            ResourcePool<VulkanBuffer> buffers;
            ResourcePool<VkSampler> samplers;
            ResourcePool<VulkanPipeline> computePipelines;
            ResourcePool<VulkanPipeline> graphicsPipelines;
        };

        void VulkanResourceCache::Shutdown()
        {
            RAW_INFO("Destroying VulkanResoureCache.");

            VkDevice device = VulkanGFXDevice::Get()->GetDevice();
            VkAllocationCallbacks* allocCallbacks = VulkanGFXDevice::Get()->GetAllocationCallbacks();

            u32 texPoolSize = textures.GetPoolSize();
            auto& aliveTextures = textures.GetBitset();
            for(u32 i = 0; i < texPoolSize; i++)
            {
                if(aliveTextures[i] == true)
                {
                    VulkanTexture* texture = textures.GetResource(i);
                    texture->Destroy();
                    textures.ReleaseResource(i);
                }
            }

            u32 bufferPoolSize = buffers.GetPoolSize();
            auto& aliveBuffers = buffers.GetBitset();
            for(u32 i = 0; i < bufferPoolSize; i++)
            {
                if(aliveBuffers[i] == true)
                {
                    VulkanBuffer* buffer = buffers.GetResource(i);
                    buffer->Destroy();
                    buffers.ReleaseResource(i);
                }
            }

            u32 samplerPoolSize = samplers.GetPoolSize();
            auto& aliveSamplers = samplers.GetBitset();
            for(u32 i = 0; i < samplerPoolSize; i++)
            {
                if(aliveSamplers[i] == true)
                {
                    VkSampler sampler = *samplers.GetResource(i);
                    vkDestroySampler(device, sampler, allocCallbacks);
                    samplers.ReleaseResource(i);
                }
            }
            
            u32 cmpPipelineSize = computePipelines.GetPoolSize();
            auto& aliveCmpPipeline = computePipelines.GetBitset();
            for(u32 i = 0; i < cmpPipelineSize; i++)
            {
                if(aliveCmpPipeline[i] == true)
                {
                    VulkanPipeline* cmpPipeline = computePipelines.GetResource(i);
                    cmpPipeline->Destroy();
                    computePipelines.ReleaseResource(i);
                }
            }

            u32 gfxPipelineSize = graphicsPipelines.GetPoolSize();
            auto& aliveGfxPipeline = graphicsPipelines.GetBitset();
            for(u32 i = 0; i < gfxPipelineSize; i++)
            {
                if(aliveGfxPipeline[i] == true)
                {
                    VulkanPipeline* gPipeline = graphicsPipelines.GetResource(i);
                    gPipeline->Destroy();
                    graphicsPipelines.ReleaseResource(i);
                }
            }

            textures.Shutdown();
            buffers.Shutdown();
            samplers.Shutdown();
            computePipelines.Shutdown();
            graphicsPipelines.Shutdown();
        }

        VulkanCommandBufferManager frameManager;
        VulkanImmediateExecutor immExecTransfer;
        VulkanImmediateExecutor immExecGFX;
        VulkanResourceCache resCache;
        TextureHandle drawImage;
        TextureHandle depthBuffer;
        VkExtent2D drawImageExtent;
        std::vector<VkCommandBuffer> submissionQueue;
        std::vector<VkCommandBuffer> alternateSubmissionQueue;
        VkSemaphore signalSubmissionQueue[MAX_SWAPCHAIN_IMAGES];
        std::mutex queueMutex;

        std::string vulkanShaderBinaries = VULKAN_SHADERS_DIR;
    }

    VulkanGFXDevice* VulkanGFXDevice::Get()
    {
        return &s_VulkanGFXDevice;
    }

    void VulkanGFXDevice::InitializeDevice(DeviceConfig& config)
    {
        RAW_INFO("Initializing VulkanGFXDevice...");

        // initialize event handlers and subscribe to events
        m_ResizeHandler = BIND_EVENT_FN(VulkanGFXDevice::OnWindowResize);

        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());
        
        // create application info        
        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = config.name.c_str();
        appInfo.pEngineName = "Raw Engine";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // create instance
        VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = ArraySize(s_RequestedExtensions);
        instanceInfo.ppEnabledExtensionNames = s_RequestedExtensions;
#if defined(_DEBUG)
        instanceInfo.enabledLayerCount = ArraySize(s_RequestedLayers);
        instanceInfo.ppEnabledLayerNames = s_RequestedLayers;

        VkDebugUtilsMessengerCreateInfoEXT debugInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugInfo.pfnUserCallback = DebugUtilsCallback;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        instanceInfo.pNext = &debugInfo;
#endif
        VK_CHECK(vkCreateInstance(&instanceInfo, m_AllocCallbacks, &m_Instance));
        RAW_INFO("Vulkan Instance successfully created.");

#if defined(_DEBUG)
        u32 numInstanceExtensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, nullptr);

        std::vector<VkExtensionProperties> extProps;
        extProps.resize(numInstanceExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, extProps.data());

        for(u32 i = 0; i < numInstanceExtensions; i++)
        {
            if(!strcmp(extProps[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
                m_DebugUtilsPresent = true;
                break;
            }
        }

        if(!m_DebugUtilsPresent)
        {
            RAW_TRACE("Vulkan Extension %s for debugging not present.", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else
        {
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT" );
            VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_Instance, &debugInfo, m_AllocCallbacks, &m_DebugMessenger));
            RAW_INFO("Vulkan DebugMessenger successfully created.");
        }
#endif

        m_SwapchainExtent.width = config.width;
        m_SwapchainExtent.height = config.height;

        // enumerate physical devices and choose appropriate device
        u32 numGPUs;
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &numGPUs, nullptr));

        std::vector<VkPhysicalDevice> gpus;
        gpus.resize(numGPUs);
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &numGPUs, gpus.data()));

        RAW_ASSERT_MSG(SDL_Vulkan_CreateSurface((SDL_Window*)config.windowHandle, m_Instance, m_AllocCallbacks, &m_Surface), "Failed to create VkSurfaceKHR");

        VkPhysicalDevice discreteGPU = VK_NULL_HANDLE;
        VkPhysicalDevice integratedGPU = VK_NULL_HANDLE;
        for(u32 i = 0; i < numGPUs; i++)
        {
            VkPhysicalDevice gpu = gpus[i];
            vkGetPhysicalDeviceProperties(gpu, &m_GPUProperties);

            if(m_GPUProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                if(GetFamilyQueue(gpu))
                {
                    discreteGPU = gpu;
                    break;
                }
                continue;
            }

            if(m_GPUProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                if(GetFamilyQueue(gpu))
                {
                    integratedGPU = gpu;
                    break;
                }
                continue;
            }
        }

        if(discreteGPU != VK_NULL_HANDLE)
        {
            m_GPU = discreteGPU;
        }
        else if(integratedGPU != VK_NULL_HANDLE)
        {
            m_GPU = integratedGPU;
        }
        else
        {
            RAW_ASSERT_MSG(false, "Suitable GPU device not found!");
        }


        RAW_INFO("Physical Device Obtained: %s", m_GPUProperties.deviceName);
        RAW_INFO("GPU Driver version: %d.%d.%d", 
            VK_API_VERSION_MAJOR(m_GPUProperties.driverVersion),
            VK_API_VERSION_MINOR(m_GPUProperties.driverVersion),
            VK_API_VERSION_PATCH(m_GPUProperties.driverVersion));
        RAW_INFO("Vulkan API version: %d.%d.%d", 
            VK_API_VERSION_MAJOR(m_GPUProperties.apiVersion),
            VK_API_VERSION_MINOR(m_GPUProperties.apiVersion),
            VK_API_VERSION_PATCH(m_GPUProperties.apiVersion));
        
        // create logical device
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_GPU, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueProps;
        queueProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_GPU, &queueFamilyCount, queueProps.data());

        m_TransferQueueFamily = U32_MAX;
        for(u32 i = 0; i < queueFamilyCount; i++)
        {
            VkQueueFamilyProperties props = queueProps[i];
            if(props.queueCount == 0) continue;
#if defined(_DEBUG)
            RAW_INFO("Family %u, flags %u queue count %u", i, props.queueFlags, props.queueCount);
#endif
            // search for main queue that should be abole to do all work (graphics, compute, transfer)
            if((props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                m_GFXQueueFamily = i;
            }

            // search for transfer queue
            if((props.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0 && (props.queueFlags & VK_QUEUE_TRANSFER_BIT))
            {
                m_TransferQueueFamily = i;
            }
        }
        
        u32 deviceExtensionCount = 1;
        const char* deviceExt[] = { "VK_KHR_swapchain" };
        const float queuePriority[] = { 1.0f };
        VkDeviceQueueCreateInfo queueInfo[2] = {};
        queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[0].queueFamilyIndex = m_GFXQueueFamily;
        queueInfo[0].queueCount = 1;
        queueInfo[0].pQueuePriorities = queuePriority;

        if(m_TransferQueueFamily < U32_MAX)
        {
            VkDeviceQueueCreateInfo& transferInfo = queueInfo[1];
            transferInfo.sType =  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            transferInfo.queueFamilyIndex = m_TransferQueueFamily;
            transferInfo.queueCount = 1;
            transferInfo.pQueuePriorities = queuePriority;
        }

        // get all features
        VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        vkGetPhysicalDeviceFeatures2(m_GPU, &features2);

        // use buffer device addressing feature 
        // use descriptor indexing
        VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features2.pNext = &features12;

        // use dynamic rendering
        VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

        features12.pNext = &features13;

        vkGetPhysicalDeviceFeatures2(m_GPU, &features2);

        RAW_ASSERT_MSG(features12.descriptorBindingPartiallyBound  && features12.runtimeDescriptorArray, "Bindless rendering not supported!");
        RAW_ASSERT_MSG(features12.bufferDeviceAddress, "Buffer device addressing not supported!");
        RAW_ASSERT_MSG(features13.dynamicRendering, "Dynamic rendering not supported!");
        RAW_ASSERT_MSG(features2.features.multiDrawIndirect, "MDI not supported!");

        VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        deviceCreateInfo.queueCreateInfoCount = m_TransferQueueFamily < U32_MAX ? 2 : 1;
        deviceCreateInfo.pQueueCreateInfos = queueInfo;
        deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExt;
        deviceCreateInfo.pNext = &features2;

        VK_CHECK(vkCreateDevice(m_GPU, &deviceCreateInfo, m_AllocCallbacks, &m_LogicalDevice));
        RAW_INFO("Vulkan Device successfully created.");

        // get function pointers to debug utils functions
        if(m_DebugUtilsPresent)
        {
            vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_LogicalDevice, "vkSetDebugUtilsObjectNameEXT");
            vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_LogicalDevice, "vkCmdBeginDebugUtilsLabelEXT");
            vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_LogicalDevice, "vkCmdEndDebugUtilsLabelEXT");
            vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        }

        vkGetDeviceQueue(m_LogicalDevice, m_GFXQueueFamily, 0, &m_GFXQueue);
        if(m_TransferQueueFamily < U32_MAX) vkGetDeviceQueue(m_LogicalDevice, m_TransferQueueFamily, 0, &m_TransferQueue);

        // select surface format
        const VkFormat surfaceImageFormats[] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR surfaceColourSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

        u32 supportedCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &supportedCount, nullptr);

        std::vector<VkSurfaceFormatKHR> formats;
        formats.resize(supportedCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &supportedCount, formats.data());
    
        bool formatFound = false;
        const u32 surfaceFormatCount = ArraySize(surfaceImageFormats);

        for(u32 i = 0; i < surfaceFormatCount; i++)
        {
            for(u32 j = 0; j < supportedCount; j++)
            {
                if(formats[j].format == surfaceImageFormats[i] && formats[j].colorSpace == surfaceColourSpace)
                {
                    m_SurfaceFormat = formats[j];
                    formatFound = true;
                    break;
                }
            }
            if(formatFound) break;
        }

        if(!formatFound) m_SurfaceFormat = formats[0];

        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> modes;
        modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &presentModeCount, modes.data());
        
        bool modeFound = false;
        VkPresentModeKHR preferredMode = VK_PRESENT_MODE_MAILBOX_KHR;
        for(u32 i = 0; i < presentModeCount; i++)
        {
            if(modes[i] == preferredMode)
            {
                m_PresentMode = modes[i];
                modeFound = true;
                break;
            }
        }

        if(!modeFound) m_PresentMode = modes[0];

        // create swapchain
        CreateSwapchain();

        // create vma allocator
        VmaAllocatorCreateInfo allocInfo = {};
        allocInfo.device = m_LogicalDevice;
        allocInfo.physicalDevice = m_GPU;
        allocInfo.instance = m_Instance;
        allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        VK_CHECK(vmaCreateAllocator(&allocInfo, &m_VmaAllocator));
        RAW_INFO("VMA Allocator successfully created.");

        // create synchronization primitives
        VkSemaphoreCreateInfo sInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            VK_CHECK(vkCreateSemaphore(m_LogicalDevice, &sInfo, m_AllocCallbacks, &m_RenderCompleteSemaphore[i]));
            VK_CHECK(vkCreateSemaphore(m_LogicalDevice, &sInfo, m_AllocCallbacks, &m_ImageAcquiredSemaphore[i]));
            VK_CHECK(vkCreateFence(m_LogicalDevice, &fInfo, m_AllocCallbacks, &m_CmdBufferExecutedFence[i]));
        }

        // create bindless descriptor pool and descriptor set layout
        VkDescriptorPoolSize bindlessPoolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VULKAN_MAX_BINDLESS_RESOURCES },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_MAX_BINDLESS_RESOURCES },
            { VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
        };

        const u32 poolSize = (u32)ArraySize(bindlessPoolSizes);
        VkDescriptorPoolCreateInfo bindlessPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        bindlessPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
        bindlessPoolInfo.maxSets = VULKAN_DEFAULT_RESOURCE_AMOUNT;
        bindlessPoolInfo.poolSizeCount = poolSize;
        bindlessPoolInfo.pPoolSizes = bindlessPoolSizes;

        VK_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &bindlessPoolInfo, m_AllocCallbacks, &m_BindlessPool));
        RAW_INFO("Bindless VkDescriptorPool successfully created.");

        std::vector<VkDescriptorSetLayoutBinding> bindlessBindings;
        bindlessBindings.resize(poolSize);
        VkDescriptorSetLayoutBinding& imageSamplerBinding = bindlessBindings[0];
        imageSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageSamplerBinding.descriptorCount = VULKAN_MAX_BINDLESS_RESOURCES;
        imageSamplerBinding.binding = VULKAN_IMAGE_SAMPLER_BINDING;
        imageSamplerBinding.stageFlags = VK_SHADER_STAGE_ALL;
        imageSamplerBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding& sampledImageBinding = bindlessBindings[1];
        sampledImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sampledImageBinding.descriptorCount = VULKAN_MAX_BINDLESS_RESOURCES;
        sampledImageBinding.binding = VULKAN_SAMPLED_IMAGE_BINDING;
        sampledImageBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding& samplerBinding = bindlessBindings[2];
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerBinding.descriptorCount = VULKAN_DEFAULT_RESOURCE_AMOUNT;
        samplerBinding.binding = VULKAN_SAMPLER_BINDING;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding& storageImageBinding = bindlessBindings[3];
        storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        storageImageBinding.descriptorCount = VULKAN_DEFAULT_RESOURCE_AMOUNT;
        storageImageBinding.binding = VULKAN_STORAGE_IMAGE_BINDING;
        storageImageBinding.stageFlags = VK_SHADER_STAGE_ALL;
        storageImageBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding& uboBinding = bindlessBindings[4];
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = VULKAN_DEFAULT_RESOURCE_AMOUNT;
        uboBinding.binding = VULKAN_UBO_BINDING;
        uboBinding.stageFlags = VK_SHADER_STAGE_ALL;
        uboBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding& ssboBinding = bindlessBindings[5];
        ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssboBinding.descriptorCount = VULKAN_DEFAULT_RESOURCE_AMOUNT;
        ssboBinding.binding = VULKAN_SSBO_BINDING;
        ssboBinding.stageFlags = VK_SHADER_STAGE_ALL;
        ssboBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo dsLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dsLayoutInfo.bindingCount = poolSize;
        dsLayoutInfo.pBindings = bindlessBindings.data();
        dsLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

        VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        bindingFlags.resize(poolSize);
        bindingFlags[0] = bindlessFlags;
        bindingFlags[1] = bindlessFlags;
        bindingFlags[2] = bindlessFlags;
        bindingFlags[3] = 0;
        bindingFlags[4] = 0;
        bindingFlags[5] = 0;

        VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        extendedInfo.bindingCount = poolSize;
        extendedInfo.pBindingFlags = bindingFlags.data();

        dsLayoutInfo.pNext = &extendedInfo;

        VK_CHECK(vkCreateDescriptorSetLayout(m_LogicalDevice, &dsLayoutInfo, m_AllocCallbacks, &m_BindlessLayout));
        RAW_INFO("Bindless VkDescriptorSetLayout successfully created.");

        VkDescriptorSetAllocateInfo setAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        setAllocInfo.descriptorPool = m_BindlessPool;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &m_BindlessLayout;

        VK_CHECK(vkAllocateDescriptorSets(m_LogicalDevice, &setAllocInfo, &m_BindlessSet));
        RAW_INFO("Bindless VkDescriptorSet successfully created.");

        // initialize scene data pool
        VkDescriptorPoolSize spSize;
        spSize.descriptorCount = 1;
        spSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        VkDescriptorPoolCreateInfo sceneDataPool = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        sceneDataPool.maxSets = MAX_SWAPCHAIN_IMAGES * 10;
        sceneDataPool.poolSizeCount = 1;
        sceneDataPool.pPoolSizes = &spSize;

        VK_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &sceneDataPool, m_AllocCallbacks, &m_SceneDataPool));

        VulkanDescriptorLayoutBuilder builder;
        builder.Init();
        builder.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
        m_SceneLayout = builder.Build();
        builder.Shutdown();

        VkDescriptorSetAllocateInfo sceneAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        sceneAllocInfo.descriptorPool = m_SceneDataPool;
        sceneAllocInfo.descriptorSetCount = 1;
        sceneAllocInfo.pSetLayouts = &m_SceneLayout;

        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            VK_CHECK(vkAllocateDescriptorSets(m_LogicalDevice, &sceneAllocInfo, &m_SceneDataSet[i]));
        }

        // initialize default descriptor pool
        VkDescriptorPoolSize defaultPoolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VULKAN_DEFAULT_RESOURCE_AMOUNT },
        };

        VkDescriptorPoolCreateInfo defaultPool = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        defaultPool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        defaultPool.maxSets = (u32)ArraySize(defaultPoolSizes) * VULKAN_DEFAULT_RESOURCE_AMOUNT;
        defaultPool.poolSizeCount = (u32)ArraySize(defaultPoolSizes);
        defaultPool.pPoolSizes = defaultPoolSizes;

        VK_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &defaultPool, m_AllocCallbacks, &m_DefaultPool));

        // initialize ring buffer
        u32 numThreads = JobSystem::GetNumThreads();
        frameManager.Init(numThreads);
        submissionQueue.reserve(frameManager.numPools);
        alternateSubmissionQueue.reserve(frameManager.numPools);
        
        VkSemaphoreCreateInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for(u32 i = 0; i < m_SwapchainImageCount; i++) vkCreateSemaphore(m_LogicalDevice, &signalInfo, m_AllocCallbacks, &signalSubmissionQueue[i]);

        // initialize immediate executor
        if(m_TransferQueueFamily < U32_MAX)
        {
            immExecTransfer.Init(m_TransferQueue, m_TransferQueueFamily);
            immExecGFX.Init(m_GFXQueue, m_GFXQueueFamily);
        }
        else
        {
            immExecTransfer.Init(m_GFXQueue, m_GFXQueueFamily);
            immExecGFX.Init(m_GFXQueue, m_GFXQueueFamily);
        }

        // initialize resource cache
        resCache.textures.Init(4096);
        resCache.buffers.Init(4096);
        resCache.samplers.Init(16);
        resCache.computePipelines.Init(128);
        resCache.graphicsPipelines.Init(128);

        // initialize default samplers
        SamplerDesc linear;
        linear.minFilter = ESamplerFilterMode::LINEAR;
        linear.magFilter = ESamplerFilterMode::LINEAR;
        linear.mipmap = ESamplerMipMapMode::LINEAR;
        linear.u = ESamplerAddressMode::REPEAT;
        linear.v = ESamplerAddressMode::REPEAT;
        linear.w = ESamplerAddressMode::REPEAT;
        linear.minLod = 0;
        linear.maxLod = LOD_CLAMP_NONE;
        
        m_LinearSampler = CreateSampler(linear);

        SamplerDesc nearest;
        nearest.minFilter = ESamplerFilterMode::NEAREST;
        nearest.magFilter = ESamplerFilterMode::NEAREST;
        nearest.mipmap = ESamplerMipMapMode::NEAREST;
        nearest.u = ESamplerAddressMode::REPEAT;
        nearest.v = ESamplerAddressMode::REPEAT;
        nearest.w = ESamplerAddressMode::REPEAT;
        nearest.minLod = 0;
        nearest.maxLod = LOD_CLAMP_NONE;

        m_NearestSampler = CreateSampler(nearest);

        SamplerDesc shadow;
        shadow.minFilter = ESamplerFilterMode::LINEAR;
        shadow.magFilter = ESamplerFilterMode::LINEAR;
        shadow.mipmap = ESamplerMipMapMode::LINEAR;
        shadow.u = ESamplerAddressMode::CLAMP_TO_BORDER;
        shadow.v = ESamplerAddressMode::CLAMP_TO_BORDER;
        shadow.w = ESamplerAddressMode::CLAMP_TO_BORDER;
        shadow.minLod = 0;
        shadow.maxLod = LOD_CLAMP_NONE;
        
        m_ShadowSampler = CreateSampler(shadow);

        // create the depth buffer and draw image
        TextureDesc drawImageDesc;
        drawImageDesc.depth = 1;
        drawImageDesc.format = vkUtils::ToTextureFormat(m_SurfaceFormat.format);
        drawImageDesc.width = config.maxWidth;
        drawImageDesc.height = config.maxHeight;
        drawImageDesc.isRenderTarget = true;
        drawImageDesc.isStorageImage = true;
        drawImageDesc.isMipmapped = false;
    
        TextureDesc depthBufferDesc;
        depthBufferDesc.depth = 1;
        depthBufferDesc.format = ETextureFormat::D32_SFLOAT;
        depthBufferDesc.width = m_SwapchainExtent.width;
        depthBufferDesc.height = m_SwapchainExtent.height;
        depthBufferDesc.isRenderTarget = false;
        depthBufferDesc.isStorageImage = false;

        drawImage = CreateTexture(drawImageDesc);
        depthBuffer = CreateTexture(depthBufferDesc, true);
    
        drawImageExtent = m_SwapchainExtent;

        SetResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)m_BindlessLayout, "Bindless Descriptor Layout");
        SetResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)m_SceneLayout, "SceneData Descriptor Layout");
        SetResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)m_BindlessSet, "Bindless Descriptor Set");

        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            cstring index = std::to_string(i).c_str();
            std::string fullName = "SceneData Descriptor Set: " + std::to_string(i); 
            SetResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)m_SceneDataSet[i], fullName.c_str());
        }
    }

    void VulkanGFXDevice::InitializeEditor()
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = (u32)ArraySize(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        VK_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, m_AllocCallbacks, &m_ImGuiPool));

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableSetMousePos;
        
        IWindow* windowInterface = (IWindow*)ServiceLocator::Get()->GetService(IWindow::k_ServiceName);
        ImGui_ImplSDL3_InitForVulkan((SDL_Window*)windowInterface->GetWindowHandle());

        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance = m_Instance;
        info.PhysicalDevice = m_GPU;
        info.Device = m_LogicalDevice;
        info.Queue = m_GFXQueue;
        info.DescriptorPool = m_ImGuiPool;
        info.MinImageCount = m_SwapchainImageCount;
        info.ImageCount = m_SwapchainImageCount;
        info.UseDynamicRendering = true;

        info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SurfaceFormat.format;

        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&info);
    }

    void VulkanGFXDevice::Shutdown()
    {
        vkDeviceWaitIdle(m_LogicalDevice);
        RAW_INFO("VulkanGFXDevice shutting down...");

        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_ResizeHandler), WindowResizeEvent::GetStaticEventType());

        frameManager.Shutdown();
        immExecTransfer.Shutdown();
        immExecGFX.Shutdown();
        resCache.Shutdown();

        RAW_INFO("Vulkan Editor shutting down...");
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        vkDestroyDescriptorPool(m_LogicalDevice, m_ImGuiPool, nullptr);

        RAW_INFO("Destroying Vulkan Bindless Pool and Layout.")
        vkDestroyDescriptorSetLayout(m_LogicalDevice, m_BindlessLayout, m_AllocCallbacks);
        vkDestroyDescriptorPool(m_LogicalDevice, m_BindlessPool, m_AllocCallbacks);

        RAW_INFO("Destroy Vulkan SceneData Pool and Layout.");
        vkDestroyDescriptorSetLayout(m_LogicalDevice, m_SceneLayout, m_AllocCallbacks);
        vkDestroyDescriptorPool(m_LogicalDevice, m_SceneDataPool, m_AllocCallbacks);

        RAW_INFO("Destroy Vulkan Default Descriptor Pool.");
        vkDestroyDescriptorPool(m_LogicalDevice, m_DefaultPool, m_AllocCallbacks);

        DestroySynchronizationPrimitives();

        DestroySwapchain();

        RAW_INFO("Destroying Vulkan Surface.");
        vkDestroySurfaceKHR(m_Instance, m_Surface, m_AllocCallbacks);

        RAW_INFO("Destroying VMA Allocator.")
        vmaDestroyAllocator(m_VmaAllocator);

        RAW_INFO("Destroying Vulkan Device.");
        vkDestroyDevice(m_LogicalDevice, m_AllocCallbacks);

#if defined(_DEBUG)
        if(m_DebugUtilsPresent)
        {
            RAW_INFO("Destroying Vulkan Debug Messenger.");
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, m_AllocCallbacks);
        }
#endif

        RAW_INFO("Destroying Vulkan Instance.");
        vkDestroyInstance(m_Instance, m_AllocCallbacks);

        RAW_INFO("VulkanGFXDevice shut down successfully.");
    }

    bool VulkanGFXDevice::GetFamilyQueue(VkPhysicalDevice gpu)
    {
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueProps;
        queueProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueProps.data());

        VkBool32 surfaceSupported;
        for(u32 i = 0; i < queueFamilyCount; i++)
        {
            VkQueueFamilyProperties props = queueProps[i];
            if(props.queueCount > 0 && props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_Surface, &surfaceSupported);

                if(surfaceSupported)
                {
                    m_GFXQueueFamily = i;
                    break;
                }
            }
        }

        return surfaceSupported;
    }

    void VulkanGFXDevice::BeginFrame()
    {
        if(m_WindowResized) RecreateSwapchain();

        VkFence* curFence = &m_CmdBufferExecutedFence[m_CurFrame];

        VkResult waitRes{ vkWaitForFences(m_LogicalDevice, 1, curFence, VK_TRUE, U64_MAX) };
        if(waitRes != VK_SUCCESS)
        {
            VK_CHECK(waitRes);
        }
        VK_CHECK(vkResetFences(m_LogicalDevice, 1, curFence));
        
        // clear the frames deletion queue at the start of the frame
        frameManager.frameDelQueue[m_CurFrame].Flush();

        VkSampler sampler = *GetSampler(m_ShadowSampler);
        frameManager.bindlessSetUpdates[m_CurFrame].WriteSampler(VULKAN_SAMPLER_BINDING, sampler);

        // update bindless descriptor set
        frameManager.bindlessSetUpdates[m_CurFrame].UpdateSet(m_BindlessSet);
        // update scene data descriptor set
        frameManager.sceneDataUpdates[m_CurFrame].UpdateSet(m_SceneDataSet[m_CurFrame]);
        
        VkResult res = vkAcquireNextImageKHR(m_LogicalDevice, m_Swapchain, U64_MAX, m_ImageAcquiredSemaphore[m_CurFrame], VK_NULL_HANDLE, &m_SwapchainImageIndex);
        if(res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RAW_DEBUG("Window resized, possibly minimized.");
        }
        
        frameManager.ResetBuffers(m_CurFrame);
        VulkanCommandBuffer* curCmdBuffer = (VulkanCommandBuffer*)GetCommandBuffer();
        curCmdBuffer->BeginCommandBuffer();

        curCmdBuffer->TransitionImage(drawImage, ETextureLayout::GENERAL);
        curCmdBuffer->TransitionImage(depthBuffer, ETextureLayout::GENERAL);
    }

    void VulkanGFXDevice::EndFrame()
    {
        VulkanCommandBuffer* curCmdBuffer = (VulkanCommandBuffer*)GetCommandBuffer();
        curCmdBuffer->BeginCommandBuffer();
        
        VulkanTexture* vulkanDrawImage = GetTexture(drawImage);
        curCmdBuffer->TransitionImage(drawImage, ETextureLayout::TRANSFER_SRC_OPTIMAL);
        vkUtils::TransitionImage(curCmdBuffer->vulkanCmdBuffer, m_SwapchainImages[m_SwapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);

        vkUtils::CopyImageToImage(curCmdBuffer->vulkanCmdBuffer, vulkanDrawImage->image, m_SwapchainImages[m_SwapchainImageIndex], drawImageExtent, m_SwapchainExtent);
        
        RenderOverlay(curCmdBuffer->vulkanCmdBuffer, m_SwapchainImageViews[m_SwapchainImageIndex]);

        vkUtils::TransitionImage(curCmdBuffer->vulkanCmdBuffer, m_SwapchainImages[m_SwapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);

        
        for(u32 i = 0; i < frameManager.threadCount; i++)
        {
            u32 index = m_CurFrame * frameManager.threadCount + i;
            for(u32 j = 0; j < frameManager.frameBuffer[index].primaryCmdBuffers.size(); j++)
            {
                if (frameManager.frameBuffer[index].primaryCmdBuffers[j].GetState() == ECommandBufferState::RECORDING)
                {
                    frameManager.frameBuffer[index].primaryCmdBuffers[j].EndCommandBuffer();
                    submissionQueue.push_back(frameManager.frameBuffer[index].primaryCmdBuffers[j].vulkanCmdBuffer);
                }
            }
        }

        VkPipelineStageFlags altWaitStages[] = { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        
        VkSubmitInfo altSignalInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        altSignalInfo.waitSemaphoreCount = 1;
        altSignalInfo.pWaitSemaphores = &m_ImageAcquiredSemaphore[m_CurFrame];
        altSignalInfo.pWaitDstStageMask = altWaitStages;
        altSignalInfo.signalSemaphoreCount = 1;
        altSignalInfo.pSignalSemaphores = &signalSubmissionQueue[m_CurFrame];
        altSignalInfo.commandBufferCount = (u32)alternateSubmissionQueue.size();
        altSignalInfo.pCommandBuffers = alternateSubmissionQueue.data();

        VkFence* curFence = &m_CmdBufferExecutedFence[m_CurFrame];
        VkSemaphore* signalSemaphore = &m_RenderCompleteSemaphore[m_CurFrame];

        VkSemaphore waitSemaphores[] = { signalSubmissionQueue[m_CurFrame] };

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphore;
        submitInfo.commandBufferCount = (u32)submissionQueue.size();
        submitInfo.pCommandBuffers = submissionQueue.data();
        submitInfo.pWaitDstStageMask = waitStages;

        VkSubmitInfo submitInfos[] = { altSignalInfo, submitInfo };

        VK_CHECK(vkQueueSubmit(m_GFXQueue, 2, submitInfos, *curFence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.swapchainCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pImageIndices = &m_SwapchainImageIndex;

        VkResult res = vkQueuePresentKHR(m_GFXQueue, &presentInfo);
        if(res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RAW_DEBUG("Window resized, possibly minimized.");
        }

        m_CurFrame = (m_CurFrame + 1) % m_SwapchainImageCount;
        submissionQueue.clear();
        alternateSubmissionQueue.clear();
    }
    
    ICommandBuffer* VulkanGFXDevice::GetCommandBuffer(bool begin)
    {
        u32 threadIndex = JobSystem::GetCurrentThread();
        return frameManager.GetCommandBuffer(m_CurFrame, threadIndex, begin);
    }

    ICommandBuffer* VulkanGFXDevice::GetSecondaryCommandBuffer()
    {
        u32 threadIndex = JobSystem::GetCurrentThread();
        return frameManager.GetSecondaryCommandBuffer(m_CurFrame, threadIndex);
    }

    void VulkanGFXDevice::MapBuffer(const BufferHandle& handle, void* data, u64 dataSize)
    {
        VulkanBuffer* buffer = GetBuffer(handle);
        RAW_ASSERT_MSG(buffer->allocInfo.size >= dataSize, "Cannot map to buffer of size %llu, data size %llu exceeds buffer size.", buffer->allocInfo.size, dataSize);
        if(buffer->usageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT == 0)
        {
            RAW_ERROR("Buffer cannot be mapped to!");
            return;
        }

        void* mappedData = nullptr;
        VK_CHECK(vmaMapMemory(m_VmaAllocator, buffer->allocation, &mappedData));
        memcpy(mappedData, data, dataSize);
    }

    void VulkanGFXDevice::UnmapBuffer(const BufferHandle& handle, bool isSceneData)
    {
        VulkanBuffer* buffer = GetBuffer(handle);
        vmaUnmapMemory(m_VmaAllocator, buffer->allocation);

        if(buffer->usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            if(isSceneData)
            {
                frameManager.sceneDataUpdates[m_CurFrame].WriteBuffer(0, buffer->buffer, buffer->allocInfo.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            }
            else
            {
                frameManager.bindlessSetUpdates[m_CurFrame].WriteBuffer(VULKAN_UBO_BINDING, buffer->buffer, buffer->allocInfo.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            }
        }
        
        if(buffer->usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            frameManager.bindlessSetUpdates[m_CurFrame].WriteBuffer(VULKAN_SSBO_BINDING, buffer->buffer, buffer->allocInfo.size, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        }
    }

    void VulkanGFXDevice::MapTexture(const TextureHandle& handle, bool isBindless)
    {
        VulkanTexture* texture = GetTexture(handle);
        VkSampler* linearSampler = GetSampler(m_LinearSampler);

        if(isBindless)
        {
            frameManager.bindlessSetUpdates[m_CurFrame].WriteImage(VULKAN_IMAGE_SAMPLER_BINDING, texture->srv, *linearSampler, texture->imageLayout, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, handle.id);
        }
    }

    void VulkanGFXDevice::CreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilites;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GPU, m_Surface, &surfaceCapabilites);

        m_SwapchainImageCount = surfaceCapabilites.minImageCount + 1;
        if(m_SwapchainImageCount > surfaceCapabilites.maxImageCount) m_SwapchainImageCount = surfaceCapabilites.maxImageCount;
        if(m_SwapchainImageCount > MAX_SWAPCHAIN_IMAGES) m_SwapchainImageCount = MAX_SWAPCHAIN_IMAGES;

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = m_Surface;
        createInfo.minImageCount = m_SwapchainImageCount;
        createInfo.imageFormat = m_SurfaceFormat.format;
        createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        createInfo.imageExtent = m_SwapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = m_PresentMode;
        createInfo.preTransform = surfaceCapabilites.currentTransform;
        createInfo.clipped = VK_TRUE;

        VK_CHECK(vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, m_AllocCallbacks, &m_Swapchain));
        RAW_INFO("Vulkan Swapchain successfully created.");

        vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_SwapchainImageCount, nullptr);
        vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages);
        
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_SurfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;

        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            viewInfo.image = m_SwapchainImages[i];
            VK_CHECK(vkCreateImageView(m_LogicalDevice, &viewInfo, m_AllocCallbacks, &m_SwapchainImageViews[i]));
        }
    }

    void VulkanGFXDevice::DestroySwapchain()
    {
        // destory swapchain chain image views
        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            vkDestroyImageView(m_LogicalDevice, m_SwapchainImageViews[i], m_AllocCallbacks);
        }

        RAW_INFO("Destroying Vulkan Swapchain.");
        // destroy swapchain
        vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, m_AllocCallbacks);
    }

    VkImage VulkanGFXDevice::GetDrawImage()
    {
        return GetTexture(drawImage)->image;
    }

    VkExtent2D VulkanGFXDevice::GetDrawExtent()
    {
        return drawImageExtent;
    }
    
    VkImage VulkanGFXDevice::GetDepthImage()
    {
        return GetTexture(depthBuffer)->image;
    }

    void VulkanGFXDevice::RecreateSwapchain()
    {
        vkDeviceWaitIdle(m_LogicalDevice);

        DestroySwapchain();
        
        RAW_INFO("Recreating Vulkan Swapchain.");

        VkSurfaceCapabilitiesKHR surfaceCapabilites;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GPU, m_Surface, &surfaceCapabilites);

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = m_Surface;
        createInfo.minImageCount = m_SwapchainImageCount;
        createInfo.imageFormat = m_SurfaceFormat.format;
        createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        createInfo.imageExtent = m_SwapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = m_PresentMode;
        createInfo.preTransform = surfaceCapabilites.currentTransform;
        createInfo.clipped = VK_TRUE;
        //createInfo.oldSwapchain = m_Swapchain;

        VK_CHECK(vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, m_AllocCallbacks, &m_Swapchain));
        RAW_INFO("Vulkan Swapchain successfully created.");
        RAW_DEBUG("Swapchain Dimensions: %u, %u", m_SwapchainExtent.width, m_SwapchainExtent.height);

        vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_SwapchainImageCount, nullptr);
        vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages);
        
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_SurfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;

        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            viewInfo.image = m_SwapchainImages[i];
            VK_CHECK(vkCreateImageView(m_LogicalDevice, &viewInfo, m_AllocCallbacks, &m_SwapchainImageViews[i]));
        }

        // recreate depth buffer
        DestroyTextureInstant(depthBuffer);

        TextureDesc depthBufferDesc;
        depthBufferDesc.depth = 1;
        depthBufferDesc.format = ETextureFormat::D32_SFLOAT;
        depthBufferDesc.width = m_SwapchainExtent.width;
        depthBufferDesc.height = m_SwapchainExtent.height;
        depthBufferDesc.isRenderTarget = false;
        depthBufferDesc.isStorageImage = false;

        depthBuffer = CreateTexture(depthBufferDesc, true);

        vkDeviceWaitIdle(m_LogicalDevice);
        
        m_WindowResized = false;
    }

    bool VulkanGFXDevice::OnWindowResize(const WindowResizeEvent& e)
    {
        m_SwapchainExtent.width = e.GetWidth();
        m_SwapchainExtent.height = e.GetHeight();
        m_WindowResized = true;
        m_WindowMinimized = false;

        VulkanTexture* drawTexture = GetTexture(drawImage);
        drawImageExtent.width = m_SwapchainExtent.width < drawTexture->imageExtent.width ? m_SwapchainExtent.width : drawTexture->imageExtent.width;
        drawImageExtent.height = m_SwapchainExtent.height < drawTexture->imageExtent.height ? m_SwapchainExtent.height : drawTexture->imageExtent.height;
        return false;
    }

    void VulkanGFXDevice::DestroySynchronizationPrimitives()
    {
        RAW_INFO("Destroying Vulkan Synchronization Primitives.")
        for(u32 i = 0; i < m_SwapchainImageCount; i++)
        {
            vkDestroySemaphore(m_LogicalDevice, m_RenderCompleteSemaphore[i], m_AllocCallbacks);
            vkDestroySemaphore(m_LogicalDevice, m_ImageAcquiredSemaphore[i], m_AllocCallbacks);
            vkDestroySemaphore(m_LogicalDevice, signalSubmissionQueue[i], m_AllocCallbacks);
            vkDestroyFence(m_LogicalDevice, m_CmdBufferExecutedFence[i], m_AllocCallbacks);
        }
    }

    TextureHandle VulkanGFXDevice::CreateTexture(const TextureDesc& desc, bool isDepth)
    {
        TextureHandle handle = { resCache.textures.ObtainResource() };
        if(!handle.IsValid())
        {
            RAW_ERROR("Invalid TextureHandle!");
            return handle;
        }
        
        VulkanTexture* vkText = resCache.textures.GetResource(handle.id);
        *vkText = {};

        VkImageCreateInfo imgInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imgInfo.format = vkUtils::ToVkFormat(desc.format);
        imgInfo.imageType = vkUtils::ToVkImageType(desc.type);
        imgInfo.extent.width = desc.width;
        imgInfo.extent.height = desc.height;
        imgInfo.extent.depth = desc.depth;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.mipLevels = !isDepth && desc.isMipmapped ? (u32)(std::floor(std::log2(std::max(desc.width, desc.height)))) + 1 : 1;
        imgInfo.arrayLayers = imgInfo.imageType == VK_IMAGE_TYPE_3D ? 6 : 1;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageUsageFlags flags = 0;
        // always default to  readable from shader
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;       
        //flags |= desc.isRenderTarget ? VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= desc.isStorageImage ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
        flags |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        imgInfo.usage = flags;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(m_VmaAllocator, &imgInfo, &allocInfo, &vkText->image, &vkText->allocation, nullptr));

        VkImageViewCreateInfo srvInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        srvInfo.format = imgInfo.format;
        srvInfo.viewType = vkUtils::ToVkImageViewType(desc.type);
        srvInfo.image = vkText->image;
        srvInfo.subresourceRange.baseMipLevel = 0;
        srvInfo.subresourceRange.levelCount = imgInfo.mipLevels;
        srvInfo.subresourceRange.baseArrayLayer = 0;
        srvInfo.subresourceRange.layerCount = imgInfo.arrayLayers;
        srvInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VK_CHECK(vkCreateImageView(m_LogicalDevice, &srvInfo, m_AllocCallbacks, &vkText->srv));

        vkText->imageExtent.width = desc.width;
        vkText->imageExtent.height = desc.height;
        vkText->imageExtent.depth = desc.depth;
        vkText->imageFormat = vkUtils::ToVkFormat(desc.format);
        vkText->isRenderTarget = desc.isRenderTarget;
        vkText->isStorageImage = desc.isStorageImage;
        vkText->isDepthTexture = isDepth;

        if(!isDepth)
        {
            VkSampler* sampler = GetSampler(m_LinearSampler);
            frameManager.bindlessSetUpdates[m_CurFrame].WriteImage(0, vkText->srv, *sampler, 
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, handle.id);
        }
        if(isDepth)
        {
            frameManager.bindlessSetUpdates[m_CurFrame].WriteImage(1, vkText->srv, VK_NULL_HANDLE, 
                VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, handle.id);
        }

        return handle;
    }
    
    TextureHandle VulkanGFXDevice::CreateTexture(const TextureDesc& desc, void* initialData)
    {
        TextureHandle newTexture = CreateTexture(desc);
        if(newTexture.IsValid())
        {
            VulkanTexture* vulkanTexture = GetTexture(newTexture);

            BufferDesc bufferDesc = {};
            bufferDesc.bufferSize = desc.width * desc.height * 4;
            bufferDesc.type = EBufferType::TRANSFER_SRC;
            bufferDesc.memoryType = EMemoryType::HOST_VISIBLE;

            BufferHandle temporaryBuffer = CreateBuffer(bufferDesc);
            VulkanBuffer* stagingBuffer = GetBuffer(temporaryBuffer);

            memcpy(stagingBuffer->allocInfo.pMappedData, initialData, bufferDesc.bufferSize);

            immExecGFX.ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = 0;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageExtent.width = desc.width;
                    copyRegion.imageExtent.height = desc.height;
                    copyRegion.imageExtent.depth = desc.depth;

                    u32 mipLevelCount = (u32)(std::floor(std::log2(std::max(desc.width, desc.height)))) + 1;
                    vkUtils::TransitionImage(cmd, vulkanTexture->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false, mipLevelCount);
                    vkCmdCopyBufferToImage(cmd, stagingBuffer->buffer, vulkanTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
                    vkUtils::GenerateMipMaps(cmd, vulkanTexture->image, { vulkanTexture->imageExtent.width, vulkanTexture->imageExtent.height });
                    vkUtils::TransitionImage(cmd, vulkanTexture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, mipLevelCount);
                    vulkanTexture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
            );

            vulkanTexture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DestroyBufferInstant(temporaryBuffer);
        }

        return newTexture;
    }

    BufferHandle VulkanGFXDevice::CreateBuffer(const BufferDesc& desc)
    {
        BufferHandle handle = { resCache.buffers.ObtainResource() };
        if(!handle.IsValid())
        {
            RAW_ERROR("Invalid BufferHandle!");
            return handle;
        }

        VulkanBuffer* vulkanBuffer = resCache.buffers.GetResource(handle.id);
        *vulkanBuffer = {};

        VkBufferUsageFlags flags = 0;
        // we use buffer device addresses for vertex buffers
        if(desc.type & EBufferType::VERTEX)
        {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        if(desc.type & EBufferType::INDEX)
        {
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if(desc.type & EBufferType::UNIFORM)                        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if(desc.type & EBufferType::STORAGE)                        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if(desc.type & EBufferType::TRANSFER_DST)                   flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if(desc.type & EBufferType::TRANSFER_SRC)                   flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if(desc.type & EBufferType::SHADER_DEVICE_ADDRESS)          flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if(desc.type & EBufferType::INDIRECT)                       flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = desc.bufferSize;
        bufferInfo.usage = flags;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = vkUtils::ToVmaMemoryUsage(desc.memoryType);
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VK_CHECK(vmaCreateBuffer(m_VmaAllocator, &bufferInfo, &allocInfo, &vulkanBuffer->buffer, &vulkanBuffer->allocation, &vulkanBuffer->allocInfo));

        if(flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo addressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addressInfo.buffer = vulkanBuffer->buffer;
            vulkanBuffer->bufferAddress = vkGetBufferDeviceAddress(m_LogicalDevice, &addressInfo);
        }
        
        vulkanBuffer->usageFlags = flags;

        return handle;
    }

    BufferHandle VulkanGFXDevice::CreateBuffer(const BufferDesc& desc, void* initialData)
    {
        BufferHandle newBuffer = CreateBuffer(desc);
        if(newBuffer.IsValid())
        {
            VulkanBuffer* buffer = GetBuffer(newBuffer);

            BufferDesc stagingDesc;
            stagingDesc.bufferSize = desc.bufferSize;
            stagingDesc.memoryType = EMemoryType::HOST_VISIBLE;
            stagingDesc.type = EBufferType::TRANSFER_SRC;

            BufferHandle stagingHandle = CreateBuffer(stagingDesc);
            VulkanBuffer* stagingBuffer = GetBuffer(stagingHandle);

            void* data = nullptr;
            VK_CHECK(vmaMapMemory(m_VmaAllocator, stagingBuffer->allocation, &data));
            memcpy(data, initialData, stagingDesc.bufferSize);
            vmaUnmapMemory(m_VmaAllocator, stagingBuffer->allocation);

            immExecTransfer.ImmediateSubmit([&](VkCommandBuffer cmd)
                {
                    VkBufferCopy bufferCopy{ 0 };
                    bufferCopy.dstOffset = 0;
                    bufferCopy.srcOffset = 0;
                    bufferCopy.size = stagingDesc.bufferSize;

                    vkCmdCopyBuffer(cmd, stagingBuffer->buffer, buffer->buffer, 1, &bufferCopy);
                }
            );
            
            DestroyBufferInstant(stagingHandle);
        }
        
        return newBuffer;
    }

    SamplerHandle VulkanGFXDevice::CreateSampler(const SamplerDesc& desc)
    {
        SamplerHandle handle = { resCache.samplers.ObtainResource() };
        if(!handle.IsValid())
        {
            RAW_ERROR("Invalid SamplerHandle!");
            return handle;
        }

        VkSampler* sampler = resCache.samplers.GetResource(handle.id);

        VkSamplerCreateInfo sInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sInfo.minFilter = vkUtils::ToVkFilter(desc.minFilter);
        sInfo.magFilter = vkUtils::ToVkFilter(desc.magFilter);
        sInfo.mipmapMode = vkUtils::ToVkSamplerMipMap(desc.mipmap);
        sInfo.addressModeU = vkUtils::ToVkSamplerAddress(desc.u);
        sInfo.addressModeV = vkUtils::ToVkSamplerAddress(desc.v);
        sInfo.addressModeW = vkUtils::ToVkSamplerAddress(desc.w);
        sInfo.minLod = desc.minLod;
        sInfo.maxLod = desc.maxLod <= VK_LOD_CLAMP_NONE ? desc.maxLod : VK_LOD_CLAMP_NONE;

        VK_CHECK(vkCreateSampler(m_LogicalDevice, &sInfo, m_AllocCallbacks, sampler));

        return handle;
    }

    ComputePipelineHandle VulkanGFXDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
    {
        ComputePipelineHandle handle = { resCache.computePipelines.ObtainResource() };
        if(!handle.IsValid())
        {
            RAW_ERROR("Invalid ComputePipelineHandle!");
            return handle;
        }

        if(desc.computeShader.stage != EShaderStage::COMPUTE)
        {
            RAW_ERROR("CreateComputePipeline requires a compute shader!");
            handle.id = INVALID_RESOURCE_HANDLE;
            return handle;
        }

        VulkanPipeline* cPipeline = resCache.computePipelines.GetResource(handle.id);
        *cPipeline = {};

        cPipeline->pipelineName = desc.name;
        VkShaderModule computeShader{ VK_NULL_HANDLE };
        ShaderDesc sDesc = desc.computeShader;
        LoadShader(sDesc, computeShader);

        RAW_ASSERT_MSG(desc.layoutDesc.numBindings <= 1, "ComputePipeline %s must have atmost 1 binding, %u bindings present!", desc.name, desc.layoutDesc.numBindings);


        if(desc.layoutDesc.numBindings > 0)
        {
            VulkanDescriptorLayoutBuilder layoutBuilder;
            layoutBuilder.Init();
    
            std::vector<u32> storageImageBindings;
            std::vector<u32> storageBufferBindings;
            std::vector<u32> uboBindings;

            if(desc.numStorageImages > 0)   storageImageBindings.resize(desc.numStorageImages);
            if(desc.numStorageBuffers > 0)  storageBufferBindings.resize(desc.numStorageBuffers);
            if(desc.numUBOs > 0)            uboBindings.resize(desc.numUBOs);
    
            u32 storageImageIndex = 0;
            u32 storageBufferIndex = 0;
            u32 uboIndex = 0;
            for(u32 i = 0; i < desc.layoutDesc.numBindings; i++)
            {
                Binding binding = desc.layoutDesc.bindings[i];
                VkDescriptorType type = vkUtils::ToVkDescriptorType(binding.type);
                layoutBuilder.AddBinding(binding.bind, binding.count, type, VK_SHADER_STAGE_COMPUTE_BIT);
                
                if(type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)    storageImageBindings[storageImageIndex] = binding.bind; storageImageIndex++;
                if(type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)   storageBufferBindings[storageBufferIndex] = binding.bind; storageBufferIndex++;
                if(type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)   uboBindings[uboIndex] = binding.bind; uboIndex++;
            }
    
            cPipeline->dsLayout = layoutBuilder.Build();
            layoutBuilder.Shutdown();
    
            VulkanDescriptorWriter writer;
            writer.Init();
            if(storageImageBindings.size() > 0)
            {
                for(u32 i = 0; i < desc.numStorageImages; i++)
                {
                    TextureHandle texHandle = desc.storageImages[i];
                    VulkanTexture* texVk = GetTexture(texHandle);
                    writer.WriteImage(storageImageBindings[i], texVk->srv, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                }
            }

            if(storageBufferBindings.size() > 0)
            {
                for(u32 i = 0; i < desc.numStorageImages; i++)
                {
                    BufferHandle bufferHandle = desc.storageBuffers[i];
                    VulkanBuffer* bufferVk = GetBuffer(bufferHandle);
                    writer.WriteBuffer(storageBufferBindings[i], bufferVk->buffer, bufferVk->allocInfo.size, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                }
            }
            
            if(uboBindings.size() > 0)
            {
                for(u32 i = 0; i < desc.numUBOs; i++)
                {
                    BufferHandle bufferHandle = desc.uniformBuffers[i];
                    VulkanBuffer* bufferVk = GetBuffer(bufferHandle);
                    writer.WriteBuffer(uboBindings[i], bufferVk->buffer, bufferVk->allocInfo.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                }
            }


            VkDescriptorSetAllocateInfo setAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            setAlloc.descriptorPool = m_DefaultPool;
            setAlloc.descriptorSetCount = 1;
            setAlloc.pSetLayouts = &cPipeline->dsLayout;
            VK_CHECK(vkAllocateDescriptorSets(m_LogicalDevice, &setAlloc, &cPipeline->set));

            writer.UpdateSet(cPipeline->set);
            writer.Shutdown();
        }

        VkPipelineLayoutCreateInfo pInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pInfo.pSetLayouts = &cPipeline->dsLayout;
        pInfo.setLayoutCount = 1;
        VK_CHECK(vkCreatePipelineLayout(m_LogicalDevice, &pInfo, m_AllocCallbacks, &cPipeline->pipelineLayout));

        VkPipelineShaderStageCreateInfo stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.pName = "main";
        stageInfo.module = computeShader;

        VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        pipelineInfo.layout = cPipeline->pipelineLayout;
        pipelineInfo.stage = stageInfo;

        VK_CHECK(vkCreateComputePipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, m_AllocCallbacks, &cPipeline->pipeline));
        RAW_DEBUG("ComputePipeline '%s' successfully created.", desc.name);

        vkDestroyShaderModule(m_LogicalDevice, computeShader, m_AllocCallbacks);

        SetResourceName(VK_OBJECT_TYPE_PIPELINE, (u64)cPipeline->pipeline, desc.name);

        return handle;
    }

    GraphicsPipelineHandle VulkanGFXDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        GraphicsPipelineHandle handle = { resCache.graphicsPipelines.ObtainResource() };
        if(!handle.IsValid())
        {
            RAW_ERROR("Invalid GraphicsPipelineHandle!");
            return handle;
        }

        VulkanPipeline* gfxPipeline = resCache.graphicsPipelines.GetResource(handle.id);

        // build shaders
        u32 numShaders = desc.sDesc.numStages;
        std::vector<VkShaderModule> shaderModules;
        shaderModules.resize(numShaders);
        // VkShaderModule* shaderModules = (VkShaderModule*)RAW_ALLOCATE(sizeof(VkShaderModule) * numShaders, 1, EMemoryTag::MEMORY_TAG_RENDERER);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.resize(numShaders);
        // VkPipelineShaderStageCreateInfo* shaderStages = (VkPipelineShaderStageCreateInfo*)RAW_ALLOCATE(sizeof(VkPipelineShaderStageCreateInfo) * numShaders, 1, EMemoryTag::MEMORY_TAG_RENDERER);

        u32 curShaderIndex = 0;
        for(u32 i = 0; i < MAX_SHADER_STAGES; i++)
        {
            ShaderDesc curShader = desc.sDesc.shaders[i];
            if(curShader.shaderName != nullptr)
            {
                RAW_ASSERT_MSG(curShader.stage != EShaderStage::COMPUTE, "Cannot include compute shaders in graphics pipeline '%s'", desc.name);
                VkShaderModule& curModule = shaderModules[curShaderIndex];
                LoadShader(curShader, curModule);
                
                shaderStages[curShaderIndex] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
                shaderStages[curShaderIndex].stage = vkUtils::ToVkShaderStage(curShader.stage);
                shaderStages[curShaderIndex].module = curModule;
                shaderStages[curShaderIndex].pName = "main";

                curShaderIndex++;
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        // use bindless descriptor set layout, scene data layout, and mesh data layout for all graphics pipelines

        if(desc.numImageAttachments > 0)
        {
            VkDescriptorSetLayout layouts[] = { m_SceneLayout, m_BindlessLayout };
            pipelineLayoutInfo.pSetLayouts = layouts;
            pipelineLayoutInfo.setLayoutCount = (u32)ArraySize(layouts);
        }
        else
        {
            VkDescriptorSetLayout layouts[] = { m_SceneLayout };
            pipelineLayoutInfo.pSetLayouts = layouts;
            pipelineLayoutInfo.setLayoutCount = (u32)ArraySize(layouts);
        }
        // push constants
        if(desc.pushConstant.size > 0)
        {
            VkPushConstantRange pConst = {};
            pConst.offset = desc.pushConstant.offset;
            pConst.size = desc.pushConstant.size;
            pConst.stageFlags = vkUtils::ToVkShaderStage(desc.pushConstant.stage);

            pipelineLayoutInfo.pPushConstantRanges = &pConst;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
        }

        VK_CHECK(vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, m_AllocCallbacks, &gfxPipeline->pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.stageCount = numShaders;
        pipelineInfo.layout = gfxPipeline->pipelineLayout;

        // using ssbo to send vertex data, leave empty
        VkPipelineVertexInputStateCreateInfo vInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        // using dynamic states for viewport/scissor, leave empty
        VkPipelineViewportStateCreateInfo vpState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vpState.viewportCount = 1; // TODO: allow for multiple viewports
        vpState.scissorCount = 1; // TODO: allow for multiple scissors
        pipelineInfo.pVertexInputState = &vInputState;
        pipelineInfo.pViewportState = &vpState;

        // input assembly state
        VkPipelineInputAssemblyStateCreateInfo iaInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        iaInfo.topology = vkUtils::ToVkTopology(desc.topology);
        iaInfo.primitiveRestartEnable = VK_FALSE; // add field to GraphicsPipelineDesc to make this toggleable

        pipelineInfo.pInputAssemblyState = &iaInfo;

        RAW_ASSERT_MSG(desc.numImageAttachments <= MAX_COLOUR_ATTACHMENTS, "GraphicsPipeline '%s' contains more than %u color attachments, %u attachments found!", 
            desc.name, MAX_COLOUR_ATTACHMENTS, desc.numImageAttachments);

        // color attachments
        std::vector<VkPipelineColorBlendAttachmentState> cAttachments;
        cAttachments.resize(desc.numImageAttachments);
        
        for(u32 i = 0; i < desc.numImageAttachments; i++)
        {
            const BlendStateDesc& blendState = desc.bsDesc[i];

            cAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            cAttachments[i].blendEnable = blendState.blendEnabled ? VK_TRUE : VK_FALSE;
            cAttachments[i].srcColorBlendFactor = vkUtils::ToVkBlendFactor(blendState.srcColour);
            cAttachments[i].dstColorBlendFactor = vkUtils::ToVkBlendFactor(blendState.dstColour);
            cAttachments[i].colorBlendOp = vkUtils::ToVkBlendOp(blendState.colourOp);
            
            if(blendState.seperateBlend)
            {
                cAttachments[i].srcAlphaBlendFactor = vkUtils::ToVkBlendFactor(blendState.srcAlpha);
                cAttachments[i].dstAlphaBlendFactor = vkUtils::ToVkBlendFactor(blendState.dstAlpha);
                cAttachments[i].alphaBlendOp = vkUtils::ToVkBlendOp(blendState.alphaOp);
            }
            else
            {
                cAttachments[i].srcAlphaBlendFactor = vkUtils::ToVkBlendFactor(blendState.srcColour);
                cAttachments[i].dstAlphaBlendFactor = vkUtils::ToVkBlendFactor(blendState.dstColour);
                cAttachments[i].alphaBlendOp = vkUtils::ToVkBlendOp(blendState.colourOp);
            }
        }

        // color blend state
        VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.logicOp = VK_LOGIC_OP_COPY;
        blendInfo.attachmentCount = desc.numImageAttachments;
        blendInfo.pAttachments = cAttachments.data();

        pipelineInfo.pColorBlendState = &blendInfo;

        // depth stencil state
        VkPipelineDepthStencilStateCreateInfo dsInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        dsInfo.depthWriteEnable = desc.dsDesc.depthWriteEnable ? VK_TRUE : VK_FALSE;
        dsInfo.stencilTestEnable = desc.dsDesc.stencilEnable ? VK_TRUE : VK_FALSE;
        dsInfo.depthTestEnable = desc.dsDesc.depthEnable ? VK_TRUE : VK_FALSE;
        dsInfo.depthCompareOp = vkUtils::ToVkCompareOp(desc.dsDesc.depthComparison);
        // TODO: add stencil

        pipelineInfo.pDepthStencilState = &dsInfo;
        
        // msaa
        VkPipelineMultisampleStateCreateInfo msaa = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        msaa.sampleShadingEnable = VK_FALSE;
        msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        msaa.minSampleShading = 1.0f;
        msaa.pSampleMask = nullptr;
        msaa.alphaToCoverageEnable = VK_FALSE; 
        msaa.alphaToOneEnable = VK_FALSE;

        pipelineInfo.pMultisampleState = &msaa;

        // dynamic rendering info
        VkPipelineRenderingCreateInfo renderingInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        renderingInfo.colorAttachmentCount = desc.numImageAttachments;
        
        std::vector<VkFormat> formats;
        if(desc.numImageAttachments > 0)
        {
            formats.resize(desc.numImageAttachments);
            for(u32 i = 0; i < desc.numImageAttachments; i++)
            {
                VulkanTexture* tex = GetTexture(desc.imageAttachments[i]);
                formats[i] = tex->imageFormat;
            }
            renderingInfo.pColorAttachmentFormats = formats.data();
        }
        else
        {
            renderingInfo.pColorAttachmentFormats = nullptr;
        }
        
        renderingInfo.depthAttachmentFormat = GetTexture(depthBuffer)->imageFormat;

        pipelineInfo.pNext = &renderingInfo;

        // dynamic state
        VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicInfo.pDynamicStates = state;
        dynamicInfo.dynamicStateCount = 2;

        pipelineInfo.pDynamicState = &dynamicInfo;

        // raseterizer
        VkPipelineRasterizationStateCreateInfo rInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rInfo.depthClampEnable = VK_FALSE;
        rInfo.rasterizerDiscardEnable = VK_FALSE;
        rInfo.polygonMode = vkUtils::ToVkPolygonMode(desc.rDesc.fillMode);
        rInfo.lineWidth = 1.0f;
        rInfo.cullMode = vkUtils::ToVkCullMode(desc.rDesc.cullMode);
        rInfo.frontFace = desc.rDesc.frontFace == EFrontFace::CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rInfo.depthBiasEnable = VK_FALSE;
        rInfo.depthBiasConstantFactor = 0.0f;
        rInfo.depthBiasClamp = 0.0f;
        rInfo.depthBiasSlopeFactor = 0.0f;

        pipelineInfo.pRasterizationState = &rInfo;

        VK_CHECK(vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, m_AllocCallbacks, &gfxPipeline->pipeline));
        RAW_DEBUG("GraphicsPipeline '%s' successfully created.", desc.name);

        gfxPipeline->pipelineName = desc.name;
        gfxPipeline->imageAttachements = desc.imageAttachments;
        gfxPipeline->numImageAttachments = desc.numImageAttachments;
        gfxPipeline->depthAttachment = desc.depthAttachment;
        // TODO: allow for custom layouts for each pipeline, first need to create DSLayout handle 
        gfxPipeline->dsLayout = VK_NULL_HANDLE;

        for(u32 i = 0; i < numShaders; i++)
        {
            vkDestroyShaderModule(m_LogicalDevice, shaderModules[i], m_AllocCallbacks);
        }

        return handle;
    }

    void VulkanGFXDevice::DestroyTexture(TextureHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid TextureHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying texture: %u", handle.id);
        
        frameManager.frameDelQueue[m_CurFrame].PushFunction(
            [=]()
            {
                VulkanTexture* texture = resCache.textures.GetResource(handle.id);
                texture->Destroy();
                resCache.textures.ReleaseResource(handle.id);
            }
        );

        handle.id = INVALID_RESOURCE_HANDLE;
    }
    
    void VulkanGFXDevice::DestroyBuffer(BufferHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid BufferHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying buffer: %u", handle.id);
        
        frameManager.frameDelQueue[m_CurFrame].PushFunction(
            [=]()
            {
                VulkanBuffer* buffer = resCache.buffers.GetResource(handle.id);
                buffer->Destroy();
                resCache.buffers.ReleaseResource(handle.id);
            }
        );

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroySampler(SamplerHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid SamplerHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying sampler: %u", handle.id);
        
        frameManager.frameDelQueue[m_CurFrame].PushFunction(
            [=, this]()
            {
                VkSampler sampler = *resCache.samplers.GetResource(handle.id);
                vkDestroySampler(m_LogicalDevice, sampler, m_AllocCallbacks);
                resCache.samplers.ReleaseResource(handle.id);
            }
        );

        handle.id = INVALID_RESOURCE_HANDLE;
    }
    
    void VulkanGFXDevice::DestroyComputePipeline(ComputePipelineHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid ComputePipelineHandle");
            return;
        }
        
        RAW_DEBUG("Destroying compute pipeline: %u", handle.id);
        
        frameManager.frameDelQueue[m_CurFrame].PushFunction(
            [=]()
            {
                VulkanPipeline pipeline = *resCache.computePipelines.GetResource(handle.id);
                pipeline.Destroy();
                resCache.computePipelines.ReleaseResource(handle.id);
            }
        );
        
        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroyGraphicsPipeline(GraphicsPipelineHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid GraphicsPipelineHandle");
            return;
        }
        
        RAW_DEBUG("Destroying graphics pipeline: %u", handle.id);
        
        frameManager.frameDelQueue[m_CurFrame].PushFunction(
            [=]()
            {
                VulkanPipeline pipeline = *resCache.graphicsPipelines.GetResource(handle.id);
                pipeline.Destroy();
                resCache.graphicsPipelines.ReleaseResource(handle.id);
            }
        );
        
        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroyTextureInstant(TextureHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid TextureHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying texture: %u", handle.id);
        
        VulkanTexture* texture = resCache.textures.GetResource(handle.id);
        texture->Destroy();
        resCache.textures.ReleaseResource(handle.id);

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroyBufferInstant(BufferHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid BufferHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying buffer: %u", handle.id);
        
        VulkanBuffer* buffer = resCache.buffers.GetResource(handle.id);
        buffer->Destroy();
        resCache.buffers.ReleaseResource(handle.id);

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroySamplerInstant(SamplerHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid SamplerHandle");
            return;
        }
        
        //RAW_DEBUG("Destroying sampler: %u", handle.id);
        
        VkSampler* sampler = resCache.samplers.GetResource(handle.id);
        vkDestroySampler(m_LogicalDevice, *sampler, m_AllocCallbacks);
        resCache.samplers.ReleaseResource(handle.id);

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroyComputePipelineInstant(ComputePipelineHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid ComputePipelineHandle");
            return;
        }
        
        RAW_DEBUG("Destroying compute pipeline: %u", handle.id);
        
        VulkanPipeline* pipeline = resCache.computePipelines.GetResource(handle.id);
        pipeline->Destroy();
        resCache.computePipelines.ReleaseResource(handle.id);

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    void VulkanGFXDevice::DestroyGraphicsPipelineInstant(GraphicsPipelineHandle& handle)
    {
        if(!handle.IsValid())
        {
            RAW_WARN("Invalid GraphicsPipelineHandle");
            return;
        }
        
        RAW_DEBUG("Destroying graphics pipeline: %u", handle.id);
        
        VulkanPipeline* pipeline = resCache.graphicsPipelines.GetResource(handle.id);
        pipeline->Destroy();
        resCache.graphicsPipelines.ReleaseResource(handle.id);

        handle.id = INVALID_RESOURCE_HANDLE;
    }

    VulkanTexture* VulkanGFXDevice::GetTexture(const TextureHandle& handle)
    {
        return resCache.textures.GetResource(handle.id);
    }
    
    VulkanBuffer* VulkanGFXDevice::GetBuffer(const BufferHandle& handle)
    {
        return resCache.buffers.GetResource(handle.id);
    }

    VkSampler* VulkanGFXDevice::GetSampler(const SamplerHandle& handle)
    {
        return resCache.samplers.GetResource(handle.id);
    }
    
    VulkanPipeline* VulkanGFXDevice::GetComputePipeline(const ComputePipelineHandle& handle)
    {
        return resCache.computePipelines.GetResource(handle.id);
    }

    VulkanPipeline* VulkanGFXDevice::GetGraphicsPipeline(const GraphicsPipelineHandle& handle)
    {
        return resCache.graphicsPipelines.GetResource(handle.id);
    }

    TextureHandle& VulkanGFXDevice::GetDrawImageHandle()
    {
        return drawImage;
    }
    
    TextureHandle& VulkanGFXDevice::GetDepthBufferHandle()
    {
        return depthBuffer;
    }

    void VulkanGFXDevice::SubmitCommandBuffer(ICommandBuffer* cmd)
    {
        std::scoped_lock<std::mutex> lock(queueMutex);

        VulkanCommandBuffer* vkCmd = (VulkanCommandBuffer*)cmd;
        vkCmd->EndCommandBuffer();
        alternateSubmissionQueue.push_back(vkCmd->vulkanCmdBuffer);
    }

    std::pair<u32, u32> VulkanGFXDevice::GetBackBufferSize()
    {
        return std::pair<u32, u32>(drawImageExtent.width, drawImageExtent.height);
    }

    void VulkanGFXDevice::LoadShader(ShaderDesc& desc, VkShaderModule& outModule)
    {
        VkShaderStageFlagBits stageFlags;
        std::string fullName = desc.shaderName;
        switch(desc.stage)
        {
            case EShaderStage::VERTEX:
            {
                stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                fullName += ".vert.spv";
                break;
            }

            case EShaderStage::FRAGMENT:
            {
                stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                fullName += ".frag.spv";
                break;
            }

            case EShaderStage::COMPUTE:
            {
                stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                fullName += ".comp.spv";
                break;
            }

            case EShaderStage::MESH:
            {
                stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
                fullName += ".mesh.spv";
                break;
            }
            
            case EShaderStage::GEOM:
            {
                stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
                fullName += ".geom.spv";
                break;
            }
            
            case EShaderStage::TESS_EVAL:
            {
                stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                fullName += ".tse.spv";
                break;
            }
            
            case EShaderStage::TESS_CONTROL:
            {
                stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                fullName += ".tsc.spv";
                break;
            }
        }

        std::string fullShaderPath = vulkanShaderBinaries + "/" + fullName;
        std::ifstream shaderFile(fullShaderPath.c_str(), std::ios::ate | std::ios::binary);

        u64 fileSize = static_cast<u64>(shaderFile.tellg());
        std::vector<char> buffer;
        buffer.reserve(fileSize);

        shaderFile.seekg(0);
        shaderFile.read(buffer.data(), fileSize);
        shaderFile.close();

        VkShaderModuleCreateInfo sInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        sInfo.codeSize = fileSize;
        sInfo.pCode = (u32*)buffer.data();

        VK_CHECK(vkCreateShaderModule(m_LogicalDevice, &sInfo, m_AllocCallbacks, &outModule));
        RAW_DEBUG("VkShaderModule '%s' successfully created.", fullName.c_str());
    }
    
    void VulkanGFXDevice::SetResourceName(VkObjectType type, u64 vulkanHandle, cstring name)
    {
        if(!m_DebugUtilsPresent) return;
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = type;
        nameInfo.objectHandle = vulkanHandle;
        nameInfo.pObjectName = name;
        vkSetDebugUtilsObjectNameEXT(m_LogicalDevice, &nameInfo);
    }

    void VulkanGFXDevice::PushMarker(VkCommandBuffer cmd, cstring name)
    {
        if(m_DebugUtilsPresent)
        {
            VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            label.pLabelName = name;
            label.color[ 0 ] = 1.0f;
            label.color[ 1 ] = 1.0f;
            label.color[ 2 ] = 1.0f;
            label.color[ 3 ] = 1.0f;
            vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
        }
    }

    void VulkanGFXDevice::PopMarker(VkCommandBuffer cmd)
    {
        if(m_DebugUtilsPresent)
        {
            vkCmdEndDebugUtilsLabelEXT(cmd); 
        }
    }

    void VulkanGFXDevice::RenderOverlay(VkCommandBuffer cmd, VkImageView curSwapchainView)
    {
        ImGui::Render();
        
        VkRenderingAttachmentInfo cAttch = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        cAttch.imageView = curSwapchainView;
        cAttch.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        cAttch.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        cAttch.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, m_SwapchainExtent };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &cAttch;
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        PushMarker(cmd, "Editor Overlay");
        vkCmdBeginRendering(cmd, &renderingInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRendering(cmd);
        PopMarker(cmd);
    }

    void VulkanGFXDevice::BeginOverlay()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        static bool demo = true;
        ImGui::ShowDemoWindow(&demo);
    }
}
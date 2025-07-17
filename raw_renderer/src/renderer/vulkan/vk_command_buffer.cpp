#include "renderer/vulkan/vk_command_buffer.hpp"
#include "renderer/vulkan/vk_gfxdevice.hpp"
#include "renderer/vulkan/vk_types.hpp"
#include "renderer/vulkan/vk_utilities.hpp"
#include "core/asserts.hpp"
#include <vector>

namespace Raw::GFX
{
    VulkanCommandBuffer::VulkanCommandBuffer()
    {
        m_State = std::make_unique<std::atomic<ECommandBufferState>>(ECommandBufferState::READY);
    }

    void VulkanCommandBuffer::Init(EQueueType type)
    {
        m_State->store(ECommandBufferState::READY);
        m_QueueType = type;
    }

    void VulkanCommandBuffer::BeginCommandBuffer()
    {
        RAW_ASSERT_MSG(m_State->load() != ECommandBufferState::CLOSED, "Command buffer in CLOSED state, cannot begin recording!");
        if(m_State->load() == ECommandBufferState::RECORDING) return; // if it's in the recording state we can just jump to recording commands

        VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(vulkanCmdBuffer, &info));
        m_State->store(ECommandBufferState::RECORDING);
    }

    void VulkanCommandBuffer::BeginSecondaryCommandBuffer()
    {
        VkCommandBufferInheritanceInfo cmdInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
        VkCommandBufferInheritanceRenderingInfo dynamicCmdInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO };
        dynamicCmdInfo.colorAttachmentCount = 0;
        dynamicCmdInfo.pColorAttachmentFormats = nullptr;
        dynamicCmdInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        dynamicCmdInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        cmdInfo.pNext = &dynamicCmdInfo;

        VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        info.pInheritanceInfo = &cmdInfo;

        VK_CHECK(vkBeginCommandBuffer(vulkanCmdBuffer, &info));
        m_State->store(ECommandBufferState::RECORDING);
    }

    void VulkanCommandBuffer::EndCommandBuffer()
    {
        VK_CHECK(vkEndCommandBuffer(vulkanCmdBuffer));
        m_State->store(ECommandBufferState::CLOSED);
    }

    void VulkanCommandBuffer::Clear(const glm::vec4& rgba)
    {
        VkClearColorValue clear = {};
        clear = { { rgba.x, rgba.y, rgba.z, rgba.w } };

        VkImageSubresourceRange clearRange = {};
        clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearRange.baseArrayLayer = 0;
        clearRange.layerCount = 1;
        clearRange.baseMipLevel = 0;
        clearRange.levelCount = 1;

        VkImage curImage = VulkanGFXDevice::Get()->GetDrawImage();
        vkCmdClearColorImage(vulkanCmdBuffer, curImage, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &clearRange);
    }
    
    void VulkanCommandBuffer::ClearDepthStencil(const glm::vec2& ds)
    {
        VkClearDepthStencilValue clear;
        clear.depth = ds.x;
        clear.stencil = (u32)ds.y;

        VkImageSubresourceRange clearRange = {};
        clearRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        clearRange.baseArrayLayer = 0;
        clearRange.layerCount = 1;
        clearRange.baseMipLevel = 0;
        clearRange.levelCount = 1;
        
        VkImage depthBuffer = VulkanGFXDevice::Get()->GetDepthImage();
        vkCmdClearDepthStencilImage(vulkanCmdBuffer, depthBuffer, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &clearRange);
    }

    void VulkanCommandBuffer::CopyImage(const TextureHandle& src, const TextureHandle& dst)
    {
        VulkanTexture* vkSrc = VulkanGFXDevice::Get()->GetTexture(src);
        VulkanTexture* vkDst = VulkanGFXDevice::Get()->GetTexture(dst);

        vkUtils::CopyImageToImage(vulkanCmdBuffer, vkSrc->image, vkDst->image, { vkSrc->imageExtent.width, vkSrc->imageExtent.height }, { vkDst->imageExtent.width, vkDst->imageExtent.height });
    }

    void VulkanCommandBuffer::Dispatch(const ComputePipelineHandle& handle, u32 groupX, u32 groupY, u32 groupZ)
    {
        vkCmdDispatch(vulkanCmdBuffer, groupX, groupY, groupZ);
        VulkanGFXDevice::Get()->PopMarker(vulkanCmdBuffer);
        activeComputePipeline = nullptr;
    }

    void VulkanCommandBuffer::TransitionImage(const TextureHandle& handle, ETextureLayout newLayout)
    {
        VulkanTexture* texture = VulkanGFXDevice::Get()->GetTexture(handle);
        vkUtils::TransitionImage(vulkanCmdBuffer, texture->image, texture->imageLayout, vkUtils::ToVkImageLayout(newLayout), texture->isDepthTexture);
        texture->imageLayout = vkUtils::ToVkImageLayout(newLayout);
    }

    void VulkanCommandBuffer::AddMemoryBarrier(EAccessFlags srcAccess, EAccessFlags dstAccess, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline)
    {
        VkAccessFlagBits srcVk = vkUtils::ToVkAccessFlags(srcAccess);
        VkAccessFlagBits dstVk = vkUtils::ToVkAccessFlags(dstAccess);
        VkPipelineStageFlags srcVkP = vkUtils::ToVkPipelineStageFlags(srcPipeline);
        VkPipelineStageFlags dstVkP = vkUtils::ToVkPipelineStageFlags(dstPipeline);
        vkUtils::AddMemoryBarrier(vulkanCmdBuffer, srcVk, dstVk, srcVkP, dstVkP);
    }

    void VulkanCommandBuffer::AddMemoryBarrier(const BufferHandle& buffer, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline)
    {
        VulkanBuffer* vBuffer = VulkanGFXDevice::Get()->GetBuffer(buffer);

        VkBufferMemoryBarrier bufferBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        bufferBarrier.buffer = vBuffer->buffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = VK_WHOLE_SIZE;

        VkPipelineStageFlags srcVkP = vkUtils::ToVkPipelineStageFlags(srcPipeline);
        VkPipelineStageFlags dstVkP = vkUtils::ToVkPipelineStageFlags(dstPipeline);

		vkCmdPipelineBarrier(vulkanCmdBuffer, srcVkP, dstVkP, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
    }

    void VulkanCommandBuffer::BeginRendering(const GraphicsPipelineHandle& handle, ERenderingOp colorOp, ERenderingOp depthOp)
    {
        VulkanPipeline* gfxPipeline = VulkanGFXDevice::Get()->GetGraphicsPipeline(handle);
        activeGraphicsPipeline = gfxPipeline;

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea = VkRect2D{ VkOffset2D{ 0, 0 }, VulkanGFXDevice::Get()->GetDrawExtent() };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = gfxPipeline->numImageAttachments;

        std::vector<VkRenderingAttachmentInfo> cAttachments;
        const u32 numAttachments = gfxPipeline->numImageAttachments;
        if(numAttachments > 0)
        {
            for(u32 i = 0; i < numAttachments; i++)
            {
                TransitionImage(gfxPipeline->imageAttachements[i], ETextureLayout::COLOR_ATTACHMENT_OPTIMAL);
            }

            cAttachments.resize(numAttachments);

            for(u32 i = 0; i < numAttachments; i++)
            {
                TextureHandle handle = gfxPipeline->imageAttachements[i];
                VulkanTexture* curTexture = VulkanGFXDevice::Get()->GetTexture(handle);
                cAttachments[i] = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
                cAttachments[i].imageView = curTexture->srv;
                cAttachments[i].imageLayout = curTexture->imageLayout;
                cAttachments[i].loadOp = vkUtils::ToVkRenderingOp(colorOp);
                cAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                if(cAttachments[i].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
                {
                    VkClearValue clearValue = {};
                    clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    clearValue.depthStencil = { 0.0f, 1 };
                    cAttachments[i].clearValue = clearValue;
                }
            }

            renderingInfo.pColorAttachments = cAttachments.data();
        }
        else
        {
            renderingInfo.pColorAttachments = nullptr;
        }
        
        if(gfxPipeline->depthAttachment)
        {
            TextureHandle handle = *gfxPipeline->depthAttachment;
            
            VulkanTexture* depthTexture = VulkanGFXDevice::Get()->GetTexture(handle);
            VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachment.imageView = depthTexture->srv;
            depthAttachment.imageLayout = depthTexture->imageLayout;
            depthAttachment.loadOp = vkUtils::ToVkRenderingOp(depthOp);
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil.depth = 1.0f;

            renderingInfo.pDepthAttachment = &depthAttachment;

            VkExtent2D drawExtent = VulkanGFXDevice::Get()->GetDrawExtent();
            if(gfxPipeline->numImageAttachments > 0)
            {
                if(drawExtent.width >= depthTexture->imageExtent.width && drawExtent.height >= depthTexture->imageExtent.height)
                {
                    renderingInfo.renderArea = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ depthTexture->imageExtent.width, depthTexture->imageExtent.height } };
                }
            }else
            {
                renderingInfo.renderArea = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ depthTexture->imageExtent.width, depthTexture->imageExtent.height } };
            }
        }
        
        VulkanGFXDevice::Get()->PushMarker(vulkanCmdBuffer, gfxPipeline->pipelineName);

        vkCmdBeginRendering(vulkanCmdBuffer, &renderingInfo);

        curRenderingInfo = renderingInfo;
    }

    void VulkanCommandBuffer::BindPipeline(const GraphicsPipelineHandle& handle)
    {
        VulkanPipeline* gfxPipeline = VulkanGFXDevice::Get()->GetGraphicsPipeline(handle);
     
        vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline->pipeline);

        const u32 numAttachments = gfxPipeline->numImageAttachments;
        u32 curFrame = VulkanGFXDevice::Get()->m_CurFrame;
        VkDescriptorSet sceneDataSet = VulkanGFXDevice::Get()->m_SceneDataSet[curFrame];
        VkDescriptorSet bindlessSet = VulkanGFXDevice::Get()->m_BindlessSet;
        VkDescriptorSet materialDataSet = VulkanGFXDevice::Get()->m_MaterialDataSet[curFrame];
        if(numAttachments > 0)
        {
            VkDescriptorSet sets[] = { sceneDataSet, bindlessSet, materialDataSet };
            vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline->pipelineLayout, 0, 3, sets, 0, nullptr);
        }
        else
        {
            VkDescriptorSet sets[] = { sceneDataSet };
            vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline->pipelineLayout, 0, 1, sets, 0, nullptr);
        }

        VkExtent2D drawExent = VulkanGFXDevice::Get()->GetDrawExtent();
        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = drawExent.width;
        viewport.height = drawExent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(vulkanCmdBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = drawExent;

        vkCmdSetScissor(vulkanCmdBuffer, 0, 1, &scissor);
    }

    void VulkanCommandBuffer::BindComputePipeline(const ComputePipelineHandle& handle)
    {
        VulkanPipeline* computePipeline = VulkanGFXDevice::Get()->GetComputePipeline(handle);
        activeComputePipeline = computePipeline;

        VulkanGFXDevice::Get()->PushMarker(vulkanCmdBuffer, computePipeline->pipelineName);

        u32 curFrame = VulkanGFXDevice::Get()->m_CurFrame;
        VkDescriptorSet sceneDataSet = VulkanGFXDevice::Get()->m_SceneDataSet[curFrame];
        VkDescriptorSet bindlessSet = VulkanGFXDevice::Get()->m_BindlessSet;
        VkDescriptorSet materialDataSet = VulkanGFXDevice::Get()->m_MaterialDataSet[curFrame];
        VkDescriptorSet sets[3] = { sceneDataSet, bindlessSet, materialDataSet };

        vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->pipeline);
        //vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->pipelineLayout, 0, 1, &computePipeline->set[VulkanGFXDevice::Get()->m_CurFrame], 0, nullptr);
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->pipelineLayout, 0, ArraySize(sets), sets, 0, nullptr);
    }

    void VulkanCommandBuffer::EndRendering()
    {
        vkCmdEndRendering(vulkanCmdBuffer);
        VulkanGFXDevice::Get()->PopMarker(vulkanCmdBuffer);
        activeGraphicsPipeline = nullptr;
        curRenderingInfo = {};
    }

    void VulkanCommandBuffer::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
    {
        vkCmdDraw(vulkanCmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
    {
        vkCmdDrawIndexed(vulkanCmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::BindVertexBuffer(const BufferHandle& vertexBuffer)
    {
        struct
        {
            VkDeviceAddress vBuffer;
        } pushConstant;

        u32 pcSize = sizeof(pushConstant);

        VulkanBuffer* buffer = VulkanGFXDevice::Get()->GetBuffer(vertexBuffer);
        pushConstant.vBuffer = buffer->bufferAddress;

        vkCmdPushConstants(vulkanCmdBuffer, activeGraphicsPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, pcSize, &pushConstant);
    }

    void VulkanCommandBuffer::BindIndexBuffer(const BufferHandle& indexBuffer)
    {
        VulkanBuffer* buffer = VulkanGFXDevice::Get()->GetBuffer(indexBuffer);
        vkCmdBindIndexBuffer(vulkanCmdBuffer, buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandBuffer::BindDrawData(glm::mat4 transform, u32 materialIndex)
    {
        struct
        {
            u32 materialIndex;
            u32 padding;
            glm::mat4 transform;
        } pushConstant;

        u32 pcSize = sizeof(pushConstant);

        pushConstant.transform = transform;
        pushConstant.materialIndex = materialIndex;

        vkCmdPushConstants(vulkanCmdBuffer, activeGraphicsPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, (u32)sizeof(VkDeviceAddress), pcSize, &pushConstant);
    }

    void VulkanCommandBuffer::BindFullScreenData(const FullScreenData& data)
    {
        struct
        {
            u32 diffuse;
            u32 roughness;
            u32 normal;
            u32 emissive;
            u32 viewspace;
            u32 lightClipSpacePos;
            u32 occlusion;
            u32 transparent;
            u32 reflection;
        } pushConstant;

        u32 pcSize = sizeof(pushConstant);

        pushConstant.diffuse = data.diffuse;
        pushConstant.roughness = data.roughness;
        pushConstant.normal = data.normal;
        pushConstant.emissive = data.emissive;
        pushConstant.viewspace = data.viewspace;
        pushConstant.lightClipSpacePos = data.lightClipSpacePos;
        pushConstant.occlusion = data.occlusion;
        pushConstant.transparent = data.transparent;
        pushConstant.reflection = data.reflection;

        vkCmdPushConstants(vulkanCmdBuffer, activeGraphicsPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, pcSize, &pushConstant);
    }

    void VulkanCommandBuffer::BindAOData(const AOData& data)
    {
        struct
        {
            u32 depth;
            u32 outputAOTexture;
            u32 normal;
            u32 diffuse;
            u32 metallicRoughness;
        } pushConstant;

        u32 pcSize = sizeof(pushConstant);

        pushConstant.depth = data.depthBuffer;
        pushConstant.outputAOTexture = data.outputAOTexture;
        pushConstant.normal = data.normalBuffer;
        pushConstant.diffuse = data.diffuse;
        pushConstant.metallicRoughness = data.metallicRoughness;

        vkCmdPushConstants(vulkanCmdBuffer, activeComputePipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pcSize, &pushConstant);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const BufferHandle& indirectBuffer, u64 offset, u32 drawCount)
    {
        VulkanBuffer* iBuffer = VulkanGFXDevice::Get()->GetBuffer(indirectBuffer);
        vkCmdDrawIndexedIndirect(vulkanCmdBuffer, iBuffer->buffer, offset, drawCount, sizeof(VkDrawIndexedIndirectCommand));
    }
}
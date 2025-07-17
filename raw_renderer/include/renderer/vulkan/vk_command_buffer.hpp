#pragma once

#include "renderer/command_buffer.hpp"
#include <vulkan/vulkan.h>

namespace Raw::GFX
{
    struct VulkanPipeline;

    class VulkanCommandBuffer final : public ICommandBuffer
    {
    public:
        VulkanCommandBuffer();

        virtual void Init(EQueueType type) override;

        virtual void BeginCommandBuffer() override;
        virtual void BeginSecondaryCommandBuffer() override;
        virtual void EndCommandBuffer() override;

        virtual void Clear(const glm::vec4& rgba) override;
        virtual void ClearDepthStencil(const glm::vec2& ds) override;
        virtual void CopyImage(const TextureHandle& src, const TextureHandle& dst) override;
        virtual void Dispatch(const ComputePipelineHandle& handle, u32 groupX, u32 groupY, u32 groupZ) override;
        virtual void TransitionImage(const TextureHandle& handle, ETextureLayout newLayout) override;
        virtual void AddMemoryBarrier(EAccessFlags srcAccess, EAccessFlags dstAccess, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline) override;
        virtual void AddMemoryBarrier(const BufferHandle& buffer, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline) override;
        virtual void BeginRendering(const GraphicsPipelineHandle& handle, ERenderingOp colorOp = ERenderingOp::LOAD, ERenderingOp depthOp = ERenderingOp::LOAD) override;
        virtual void BindPipeline(const GraphicsPipelineHandle& handle) override;
        virtual void BindComputePipeline(const ComputePipelineHandle& handle) override;
        virtual void EndRendering() override;
        virtual void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) override;
        virtual void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;
        virtual void DrawIndexedIndirect(const BufferHandle& indirectBuffer, u64 offset, u32 drawCount) override;
        virtual void BindVertexBuffer(const BufferHandle& vertexBuffer) override;
        virtual void BindDrawData(glm::mat4 transform = glm::mat4(1.f), u32 materialIndex = U32_MAX) override;
        virtual void BindIndexBuffer(const BufferHandle& indexBuffer) override;
        virtual void BindFullScreenData(const FullScreenData& data) override;
        virtual void BindAOData(const AOData& data) override;

        
        VkCommandBuffer vulkanCmdBuffer{ VK_NULL_HANDLE };
        VulkanPipeline* activeGraphicsPipeline{ nullptr };
        VulkanPipeline* activeComputePipeline{ nullptr };
        VkRenderingInfo curRenderingInfo{};
        
    }; 
}
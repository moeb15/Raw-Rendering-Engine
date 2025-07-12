#pragma once

#include "renderer/gpu_resources.hpp"
#include "renderer/renderer_data.hpp"
#include <memory>
#include <atomic>
#include <glm/glm.hpp>

namespace Raw::GFX
{
    class ICommandBuffer
    {
    public:
        virtual void Init(EQueueType type) = 0;
        virtual void BeginCommandBuffer() = 0;
        virtual void BeginSecondaryCommandBuffer() = 0;
        virtual void EndCommandBuffer() = 0;

        virtual void Clear(const glm::vec4& rgba) = 0;
        virtual void ClearDepthStencil(const glm::vec2& ds) = 0;
        virtual void CopyImage(const TextureHandle& src, const TextureHandle& dst) = 0;
        virtual void Dispatch(const ComputePipelineHandle& handle, u32 groupX, u32 groupY, u32 groupZ) = 0;
        virtual void TransitionImage(const TextureHandle& handle, ETextureLayout newLayout) = 0;
        virtual void AddMemoryBarrier(EAccessFlags srcAccess, EAccessFlags dstAccess, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline) = 0;
        virtual void AddMemoryBarrier(const BufferHandle& buffer, EPipelineStageFlags srcPipeline, EPipelineStageFlags dstPipeline) = 0;
        virtual void BeginRendering(const GraphicsPipelineHandle& handle, bool useDepth = false, bool clearAttachments = false, bool writeDepth = false) = 0;
        virtual void BindPipeline(const GraphicsPipelineHandle& handle) = 0;
        virtual void EndRendering() = 0;
        virtual void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) = 0;
        virtual void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) = 0;
        virtual void DrawIndexedIndirect(const BufferHandle& indirectBuffer, u64 offset, u32 drawCount) = 0;
        virtual void BindVertexBuffer(const BufferHandle& vertexBuffer, glm::mat4 transform = glm::mat4(1.f), u32 materialIndex = U32_MAX) = 0;
        virtual void BindIndexBuffer(const BufferHandle& indexBuffer) = 0;
        virtual void BindFullScreenData(const FullScreenData& data) = 0;
        
        ECommandBufferState GetState() const { return m_State->load(); }
        EQueueType GetQueueType() const { return m_QueueType; }

        void Reset() { m_State->store(ECommandBufferState::READY); }

    protected:
        std::unique_ptr<std::atomic<ECommandBufferState>> m_State{ nullptr };
        EQueueType m_QueueType{ EQueueType::GRAPHICS };

    };
}
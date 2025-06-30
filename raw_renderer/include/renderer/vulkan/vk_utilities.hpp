#pragma once

#include "renderer/gpu_resources.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Raw::GFX::vkUtils
{
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout curLayout, VkImageLayout newLayout, bool isDepth, u32 mipLevel = 1, u32 srcQueue = 0, u32 dstQueue = 0);
	void CopyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
	void GenerateMipMaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
	void AddMemoryBarrier(VkCommandBuffer cmd, VkAccessFlagBits src, VkAccessFlagBits dst, VkPipelineStageFlags srcP, VkPipelineStageFlags dstP);

	VkAccessFlagBits ToVkAccessFlags(EAccessFlags flag);
	VkPipelineStageFlags ToVkPipelineStageFlags(EPipelineStageFlags flag);

	VkImageType ToVkImageType(ETextureType type);
	VkImageViewType ToVkImageViewType(ETextureType type);
	VkFormat ToVkFormat(ETextureFormat format);
	ETextureFormat ToTextureFormat(VkFormat format);

	VmaMemoryUsage ToVmaMemoryUsage(EMemoryType type);
	VkFilter ToVkFilter(ESamplerFilterMode filter);
	VkSamplerMipmapMode ToVkSamplerMipMap(ESamplerMipMapMode mipmap);
	VkSamplerAddressMode ToVkSamplerAddress(ESamplerAddressMode addressMode);
	VkBlendFactor ToVkBlendFactor(EBlendState state);
	VkBlendOp ToVkBlendOp(EBlendOp op);
	VkStencilOp ToVkStencilOp(EStencilOp op);
	VkPrimitiveTopology ToVkTopology(EPrimitiveTopology topology);
	VkCompareOp ToVkCompareOp(EComparisonFunc func);
	VkCullModeFlagBits ToVkCullMode(ECullMode mode);
	VkDescriptorType ToVkDescriptorType(EDescriptorType type);
	VkShaderStageFlagBits ToVkShaderStage(EShaderStage stage);
	VkPolygonMode ToVkPolygonMode(EFillMode fillMode);
	VkImageLayout ToVkImageLayout(ETextureLayout layout);
}
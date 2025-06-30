#include "renderer/vulkan/vk_utilities.hpp"
#include <algorithm>
#include <cmath>

namespace Raw::GFX::vkUtils
{
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout curLayout, VkImageLayout newLayout, bool isDepth, u32 mipLevel, u32 srcQueue, u32 dstQueue)
    {
		VkImageMemoryBarrier2 imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.srcQueueFamilyIndex = srcQueue;
		imageBarrier.dstQueueFamilyIndex = dstQueue;

		imageBarrier.oldLayout = curLayout;
		imageBarrier.newLayout = newLayout;

		VkImageAspectFlags aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.aspectMask = aspectMask;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = mipLevel;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
    }

	void CopyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize)
	{
		VkImageBlit2 blitRegion{};
		blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		blitRegion.pNext = nullptr;

		blitRegion.srcOffsets[1].x = srcSize.width;
		blitRegion.srcOffsets[1].y = srcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = dstSize.width;
		blitRegion.dstOffsets[1].y = dstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo{};
		blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
		blitInfo.pNext = nullptr;
		blitInfo.dstImage = dst;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = src;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2(cmd, &blitInfo);
	}

	void GenerateMipMaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize)
	{
		u32 mipLevels = (u32)(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
		for(u32 mip = 0; mip < mipLevels; mip++)
		{
			VkExtent2D halfSize = imageSize;
			halfSize.width /= 2;
			halfSize.height /= 2;

			VkImageMemoryBarrier2 imageBarrier{};
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageBarrier.pNext = nullptr;

			imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
			imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrier.subresourceRange.aspectMask = aspectMask;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseMipLevel = mip;
			imageBarrier.image = image;

			VkDependencyInfo depInfo{};
			depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			depInfo.pNext = nullptr;
			depInfo.imageMemoryBarrierCount = 1;
			depInfo.pImageMemoryBarriers = &imageBarrier;

			vkCmdPipelineBarrier2(cmd, &depInfo);

			if (mip < mipLevels - 1)
			{
				VkImageBlit2 blitRegion{};
				blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
				blitRegion.pNext = nullptr;

				blitRegion.srcOffsets[1].x = imageSize.width;
				blitRegion.srcOffsets[1].y = imageSize.height;
				blitRegion.srcOffsets[1].z = 1;

				blitRegion.dstOffsets[1].x = halfSize.width;
				blitRegion.dstOffsets[1].y = halfSize.height;
				blitRegion.dstOffsets[1].z = 1;

				blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitRegion.srcSubresource.baseArrayLayer = 0;
				blitRegion.srcSubresource.layerCount = 1;
				blitRegion.srcSubresource.mipLevel = mip;

				blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitRegion.dstSubresource.baseArrayLayer = 0;
				blitRegion.dstSubresource.layerCount = 1;
				blitRegion.dstSubresource.mipLevel = mip + 1;

				VkBlitImageInfo2 blitInfo{};
				blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
				blitInfo.pNext = nullptr;
				blitInfo.dstImage = image;
				blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitInfo.srcImage = image;
				blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				blitInfo.filter = VK_FILTER_LINEAR;
				blitInfo.regionCount = 1;
				blitInfo.pRegions = &blitRegion;

				vkCmdBlitImage2(cmd, &blitInfo);

				imageSize = halfSize;
			}
		}
	}

	void AddMemoryBarrier(VkCommandBuffer cmd, VkAccessFlagBits src, VkAccessFlagBits dst, VkPipelineStageFlags srcP, VkPipelineStageFlags dstP)
	{
		VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		memoryBarrier.srcAccessMask = src;
		memoryBarrier.dstAccessMask = dst;

		vkCmdPipelineBarrier(cmd, srcP, dstP, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	VkImageType ToVkImageType(ETextureType type)
	{
		switch(type)
		{
			case ETextureType::TEXTURE1D: 								return VK_IMAGE_TYPE_1D;
			case ETextureType::TEXTURE2D: 								return VK_IMAGE_TYPE_2D;
			case ETextureType::TEXTURE3D: 								return VK_IMAGE_TYPE_3D;
			default: 													return VK_IMAGE_TYPE_2D;
		}
	}

	VkAccessFlagBits ToVkAccessFlags(EAccessFlags flag)
	{
		switch(flag)
		{
			case EAccessFlags::COLOR_ATTACHMENT_READ_BIT:				return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			case EAccessFlags::COLOR_ATTACHMENT_WRITE_BIT:				return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			case EAccessFlags::DEPTH_STENCIL_ATTACHMENT_READ_BIT:		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			case EAccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			default:													return VK_ACCESS_FLAG_BITS_MAX_ENUM;
		}
	}

	VkPipelineStageFlags ToVkPipelineStageFlags(EPipelineStageFlags flag)
	{
		switch(flag)
		{
			case EPipelineStageFlags::VERTEX_INPUT_BIT:						return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			case EPipelineStageFlags::VERTEX_SHADER_BIT:					return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			case EPipelineStageFlags::TESSELLATION_CONTROL_SHADER_BIT:		return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
			case EPipelineStageFlags::TESSELLATION_EVALUATION_SHADER_BIT:	return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			case EPipelineStageFlags::GEOMETRY_SHADER_BIT:					return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			case EPipelineStageFlags::FRAGMENT_SHADER_BIT:					return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			case EPipelineStageFlags::EARLY_FRAGMENT_TESTS_BIT:				return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			case EPipelineStageFlags::LATE_FRAGMENT_TESTS_BIT:				return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			case EPipelineStageFlags::COLOR_ATTACHMENT_OUTPUT_BIT:			return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			case EPipelineStageFlags::NONE:									return VK_PIPELINE_STAGE_NONE;
			default:														return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	VkImageViewType ToVkImageViewType(ETextureType type)
	{
		switch(type)
		{
			case ETextureType::TEXTURE1D: 								return VK_IMAGE_VIEW_TYPE_1D;
			case ETextureType::TEXTURE2D: 								return VK_IMAGE_VIEW_TYPE_2D;
			case ETextureType::TEXTURE3D: 								return VK_IMAGE_VIEW_TYPE_3D;
			case ETextureType::TEXTURE_CUBE: 							return VK_IMAGE_VIEW_TYPE_CUBE;
			case ETextureType::TEXTURE1D_ARRAY: 						return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			case ETextureType::TEXTURE2D_ARRAY: 						return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			case ETextureType::TEXTURE_CUBE_ARRAY: 						return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			default: 													return VK_IMAGE_VIEW_TYPE_2D;
		}
	}
	
	VkFormat ToVkFormat(ETextureFormat format)
	{
		switch(format)
		{
			case ETextureFormat::UNDEFINED: 							return VK_FORMAT_UNDEFINED;
			case ETextureFormat::R8G8B8A8_UNORM: 						return VK_FORMAT_R8G8B8A8_UNORM;
			case ETextureFormat::B8G8R8A8_UNORM:						return VK_FORMAT_B8G8R8A8_UNORM;
			case ETextureFormat::R16G16B16_UNORM:						return VK_FORMAT_R16G16B16A16_UNORM;
			case ETextureFormat::D32_SFLOAT: 							return VK_FORMAT_D32_SFLOAT;
			case ETextureFormat::D24_UNORM_S8_UINT: 					return VK_FORMAT_D24_UNORM_S8_UINT;
			case ETextureFormat::D32_SFLOAT_S8_UINT: 					return VK_FORMAT_D32_SFLOAT_S8_UINT;
			default:													return VK_FORMAT_UNDEFINED;
		}
	}

	ETextureFormat ToTextureFormat(VkFormat format)
	{
		switch(format)
		{
			case VK_FORMAT_UNDEFINED: 									return ETextureFormat::UNDEFINED;
			case VK_FORMAT_R8G8B8A8_UNORM:								return ETextureFormat::R8G8B8A8_UNORM;
			case VK_FORMAT_B8G8R8A8_UNORM:								return ETextureFormat::B8G8R8A8_UNORM;
			case VK_FORMAT_R16G16B16A16_UNORM:							return ETextureFormat::R16G16B16_UNORM;
			case VK_FORMAT_D32_SFLOAT: 									return ETextureFormat::D32_SFLOAT;
			case VK_FORMAT_D24_UNORM_S8_UINT:							return ETextureFormat::D24_UNORM_S8_UINT;
			case VK_FORMAT_D32_SFLOAT_S8_UINT:							return ETextureFormat::D32_SFLOAT_S8_UINT;
			default:													return ETextureFormat::UNDEFINED;
		}
	}

	VmaMemoryUsage ToVmaMemoryUsage(EMemoryType type)
	{
		switch(type)
		{
			case EMemoryType::DEVICE_LOCAL: 							return VMA_MEMORY_USAGE_GPU_ONLY;
			case EMemoryType::HOST_VISIBLE: 							return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case EMemoryType::HOST_COHERENT: 							return VMA_MEMORY_USAGE_CPU_ONLY;
			default:													return VMA_MEMORY_USAGE_CPU_ONLY;
		}
	}

	VkFilter ToVkFilter(ESamplerFilterMode filter)
	{
		switch(filter)
		{
			case ESamplerFilterMode::LINEAR: 							return VK_FILTER_LINEAR;
			case ESamplerFilterMode::NEAREST: 							return VK_FILTER_NEAREST;
			case ESamplerFilterMode::CUBIC: 							return VK_FILTER_CUBIC_EXT;
			default:													return VK_FILTER_NEAREST;
		}
	}
	
	VkSamplerMipmapMode ToVkSamplerMipMap(ESamplerMipMapMode mipmap)
	{
		switch(mipmap)
		{
			case ESamplerMipMapMode::NEAREST: 							return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case ESamplerMipMapMode::LINEAR: 							return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			default:													return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		}
	}
	
	VkSamplerAddressMode ToVkSamplerAddress(ESamplerAddressMode addressMode)
	{
		switch(addressMode)
		{
			case ESamplerAddressMode::REPEAT:							return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case ESamplerAddressMode::MIRRORED_REPEAT:					return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case ESamplerAddressMode::CLAMP_TO_EDGE: 					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case ESamplerAddressMode::CLAMP_TO_BORDER: 					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			default:													return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkBlendFactor ToVkBlendFactor(EBlendState state)
	{
		switch(state)
		{
			case EBlendState::ZERO:										return VK_BLEND_FACTOR_ZERO;
        	case EBlendState::ONE: 										return VK_BLEND_FACTOR_ONE;
        	case EBlendState::SRC_COLOR: 								return VK_BLEND_FACTOR_SRC_COLOR;
        	case EBlendState::ONE_MINUS_SRC_COLOR: 						return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        	case EBlendState::DST_COLOR: 								return VK_BLEND_FACTOR_DST_COLOR;
        	case EBlendState::ONE_MINUS_DST_COLOR: 						return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        	case EBlendState::SRC_ALPHA: 								return VK_BLEND_FACTOR_SRC_ALPHA;
        	case EBlendState::ONE_MINUS_SRC_ALPHA: 						return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        	case EBlendState::DST_ALPHA: 								return VK_BLEND_FACTOR_DST_ALPHA;
        	case EBlendState::ONE_MINUS_DST_ALPHA: 						return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        	case EBlendState::CONSTANT_COLOR: 							return VK_BLEND_FACTOR_CONSTANT_COLOR;
        	case EBlendState::ONE_MINUS_CONSTANT_COLOR: 				return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        	case EBlendState::CONSTANT_ALPHA: 							return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        	case EBlendState::ONE_MINUS_CONSTANT_ALPHA: 				return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        	case EBlendState::SRC_ALPHA_SATURATE: 						return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        	case EBlendState::SRC1_COLOR: 								return VK_BLEND_FACTOR_SRC1_COLOR;
        	case EBlendState::ONE_MINUS_SRC1_COLOR: 					return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        	case EBlendState::SRC1_ALPHA: 								return VK_BLEND_FACTOR_SRC1_ALPHA;
        	case EBlendState::ONE_MINUS_SRC1_ALPHA: 					return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
			default:													return VK_BLEND_FACTOR_ZERO;
		}
	}
	
	VkBlendOp ToVkBlendOp(EBlendOp op)
	{
		switch(op)
		{
			case EBlendOp::ADD:											return VK_BLEND_OP_ADD; 
        	case EBlendOp::SUBTRACT: 									return VK_BLEND_OP_SUBTRACT;
        	case EBlendOp::REVERSE_SUBTRACT: 							return VK_BLEND_OP_REVERSE_SUBTRACT;
        	case EBlendOp::MIN: 										return VK_BLEND_OP_MIN;
        	case EBlendOp::MAX: 										return VK_BLEND_OP_MAX;
			default:													return VK_BLEND_OP_ADD;
		}
	}

	VkStencilOp ToVkStencilOp(EStencilOp op)
	{
		switch(op)
		{
			case EStencilOp::KEEP:										return VK_STENCIL_OP_KEEP;
        	case EStencilOp::ZERO:										return VK_STENCIL_OP_ZERO;
        	case EStencilOp::REPLACE:									return VK_STENCIL_OP_REPLACE;
        	case EStencilOp::INCREMENT_AND_CLAMP:						return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        	case EStencilOp::DECREMENT_AND_CLAMP:						return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        	case EStencilOp::INVERT:									return VK_STENCIL_OP_INVERT;
        	case EStencilOp::INCREMENT_AND_WRAP:						return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        	case EStencilOp::DECREMENT_AND_WRAP:						return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			default:													return VK_STENCIL_OP_KEEP;
		}
	}

	VkPrimitiveTopology ToVkTopology(EPrimitiveTopology topology)
	{
		switch(topology)
		{
			case EPrimitiveTopology::POINT:								return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        	case EPrimitiveTopology::LINE:								return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        	case EPrimitiveTopology::TRIANGLE:							return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        	case EPrimitiveTopology::PATCH:								return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			default:													return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkCompareOp ToVkCompareOp(EComparisonFunc func)
	{
		switch(func)
		{
			case EComparisonFunc::NEVER:								return VK_COMPARE_OP_NEVER;
			case EComparisonFunc::LESS:									return VK_COMPARE_OP_LESS;
			case EComparisonFunc::EQUAL:								return VK_COMPARE_OP_EQUAL;
			case EComparisonFunc::LESS_EQUAL:							return VK_COMPARE_OP_LESS_OR_EQUAL;
			case EComparisonFunc::GREATER:								return VK_COMPARE_OP_GREATER;
			case EComparisonFunc::GREATER_EQUAL:						return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case EComparisonFunc::NOT_EQUAL:							return VK_COMPARE_OP_NOT_EQUAL;
			case EComparisonFunc::ALWAYS:								return VK_COMPARE_OP_ALWAYS;
			default:													return VK_COMPARE_OP_LESS_OR_EQUAL;
		}
	}

	VkCullModeFlagBits ToVkCullMode(ECullMode mode)
	{
		switch(mode)
		{
			case ECullMode::NONE:										return VK_CULL_MODE_NONE;
        	case ECullMode::FRONT:										return VK_CULL_MODE_FRONT_BIT;
        	case ECullMode::BACK:										return VK_CULL_MODE_BACK_BIT;
        	case ECullMode::FRONT_AND_BACK:								return VK_CULL_MODE_FRONT_AND_BACK;
			default:													return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkDescriptorType ToVkDescriptorType(EDescriptorType type)
	{
		switch(type)
		{
			case EDescriptorType::SAMPLER:								return VK_DESCRIPTOR_TYPE_SAMPLER;	 
        	case EDescriptorType::IMAGE_SAMPLER: 						return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        	case EDescriptorType::SAMPLED_IMAGE: 						return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        	case EDescriptorType::STORAGE_IMAGE: 						return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        	case EDescriptorType::UNIFORM_TEXEL_BUFFER: 				return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        	case EDescriptorType::STORAGE_TEXEL_BUFFER: 				return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        	case EDescriptorType::UNIFORM_BUFFER: 						return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        	case EDescriptorType::STORAGE_BUFFER: 						return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        	case EDescriptorType::UNIFORM_DYNAMIC: 						return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        	case EDescriptorType::STORAGE_DYNAMIC: 						return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			default:													return VK_DESCRIPTOR_TYPE_SAMPLER;
		}
	}

	VkShaderStageFlagBits ToVkShaderStage(EShaderStage stage)
	{
		switch(stage)
		{
			case EShaderStage::VERTEX:									return VK_SHADER_STAGE_VERTEX_BIT;
        	case EShaderStage::FRAGMENT:								return VK_SHADER_STAGE_FRAGMENT_BIT;
        	case EShaderStage::COMPUTE:									return VK_SHADER_STAGE_COMPUTE_BIT;
        	case EShaderStage::MESH:									return VK_SHADER_STAGE_MESH_BIT_EXT;
        	case EShaderStage::GEOM:									return VK_SHADER_STAGE_GEOMETRY_BIT;
        	case EShaderStage::TESS_EVAL:								return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        	case EShaderStage::TESS_CONTROL:							return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			default:													return VK_SHADER_STAGE_ALL;
		}
	}

	VkPolygonMode ToVkPolygonMode(EFillMode fillMode)
	{
		switch(fillMode)
		{
			case EFillMode::WIREFRAME:									return VK_POLYGON_MODE_LINE;
        	case EFillMode::SOLID:										return VK_POLYGON_MODE_FILL;
        	case EFillMode::POINT:										return VK_POLYGON_MODE_POINT;
			default:													return VK_POLYGON_MODE_LINE;
		}
	}

	VkImageLayout ToVkImageLayout(ETextureLayout layout)
	{
		switch(layout)
		{
			case ETextureLayout::UNDEFINED:								return VK_IMAGE_LAYOUT_UNDEFINED;
        	case ETextureLayout::GENERAL:								return VK_IMAGE_LAYOUT_GENERAL;
        	case ETextureLayout::COLOR_ATTACHMENT_OPTIMAL:				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        	case ETextureLayout::DEPTH_ATTACHMENT_OPTIMAL:				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        	case ETextureLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        	case ETextureLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        	case ETextureLayout::SHADER_READ_ONLY_OPTIMAL:				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        	case ETextureLayout::TRANSFER_SRC_OPTIMAL:					return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        	case ETextureLayout::TRANSFER_DST_OPTIMAL:					return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			default:													return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}
}
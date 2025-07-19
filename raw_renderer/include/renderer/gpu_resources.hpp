#pragma once

#include "core/defines.hpp"

namespace Raw::GFX
{
    constexpr u32 MAX_DESCRIPTORS = 32;
    constexpr u32 MAX_SHADER_STAGES = 7;
    constexpr u32 MAX_COLOUR_ATTACHMENTS = 8;
    constexpr f32 LOD_CLAMP_NONE = 1000.0f; 
    constexpr u32 MAX_MATERIALS = 512;

    constexpr u32 INVALID_RESOURCE_HANDLE = U32_MAX;
    typedef u32 ResourceHandle;

    struct TextureHandle
    {
        ResourceHandle id{ INVALID_RESOURCE_HANDLE };
        bool IsValid() const { return id != INVALID_RESOURCE_HANDLE; }
    };

    struct BufferHandle
    {
        ResourceHandle id{ INVALID_RESOURCE_HANDLE };
        bool IsValid() const { return id != INVALID_RESOURCE_HANDLE; }
    };

    struct SamplerHandle
    {
        ResourceHandle id{ INVALID_RESOURCE_HANDLE };
        bool IsValid() const { return id != INVALID_RESOURCE_HANDLE; }
    };

    struct GraphicsPipelineHandle
    {
        ResourceHandle id{ INVALID_RESOURCE_HANDLE };
        bool IsValid() const { return id != INVALID_RESOURCE_HANDLE; }
    };

    struct ComputePipelineHandle
    {
        ResourceHandle id{ INVALID_RESOURCE_HANDLE };
        bool IsValid() const { return id != INVALID_RESOURCE_HANDLE; }
    };

    enum class EGPUResourceType : u8
    {
        TEXTURE,
        BUFFER,
    };

    enum class EQueueType : u8
    {
        GRAPHICS,
        COMPUTE,
        TRANSFER,
    };

    enum class ECommandBufferState : u8
    {
        READY,
        RECORDING,
        CLOSED,
    };

    enum class ECullMode : u8
    {
        NONE,
        FRONT,
        BACK,
        FRONT_AND_BACK,
    };

    enum class EFrontFace : u8
    {
        COUNTER_CLOCKWISE,
        CLOCKWISE,
    };

    enum class EPrimitiveTopology : u8
    {
        POINT,
        LINE,
        TRIANGLE,
        PATCH,
    };

    enum class EComparisonFunc : u8
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        GREATER_EQUAL,
        NOT_EQUAL,
        ALWAYS,
    };

    enum class EStencilOp : u8
    {
        KEEP,
        ZERO,
        REPLACE,
        INCREMENT_AND_CLAMP,
        DECREMENT_AND_CLAMP,
        INVERT,
        INCREMENT_AND_WRAP,
        DECREMENT_AND_WRAP,
    };

    enum class ETextureType : u8
    {
        TEXTURE1D,
        TEXTURE2D,  
        TEXTURE3D,
        TEXTURE_CUBE,
        TEXTURE1D_ARRAY,  
        TEXTURE2D_ARRAY,  
        TEXTURE_CUBE_ARRAY,
    };

    enum class ETextureFormat : u8
    {
        UNDEFINED,
        R8G8B8A8_UNORM,
        B8G8R8A8_UNORM,
        R16G16B16_UNORM,
        D32_SFLOAT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT,
    };

    enum class EMemoryType : u8
    {
        HOST_VISIBLE,
        HOST_COHERENT,
        DEVICE_LOCAL,
    };

    enum EBufferType
    {
        INVALID = 0,
        INDEX = 1 << 0,
        VERTEX = 1 << 1,
        UNIFORM = 1 << 2,
        STORAGE = 1 << 3,
        TRANSFER_DST = 1 << 4,
        TRANSFER_SRC = 1 << 5,
        SHADER_DEVICE_ADDRESS = 1 << 6,
        INDIRECT = 1 << 7,
    };

    enum class EDepthWriteMask : u8
    {
        ZERO,
        ALL
    };

    enum class EFillMode : u8
    {
        WIREFRAME,
        SOLID,
        POINT
    };

    enum class ESamplerFilterMode : u8
    {
        NEAREST,
        LINEAR,
        CUBIC,
    };

    enum class ESamplerMipMapMode : u8
    {
        NEAREST,
        LINEAR,
    };

    enum class ESamplerAddressMode : u8
    {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER,
    };

    enum EShaderStage : u8
    {
        INVALID_SHADER_STAGE                  = 0,
        VERTEX_STAGE                          = 1 << 0,
        FRAGMENT_STAGE                        = 1 << 1,
        COMPUTE_STAGE                         = 1 << 2,
        MESH_STAGE                            = 1 << 3,
        GEOM_STAGE                            = 1 << 4,
        TESS_EVAL_STAGE                       = 1 << 5,
        TESS_CONTROL_STAGE                    = 1 << 6,
    };

    enum class EBlendState : u8
    {
        ZERO, 
        ONE, 
        SRC_COLOR, 
        ONE_MINUS_SRC_COLOR, 
        DST_COLOR, 
        ONE_MINUS_DST_COLOR, 
        SRC_ALPHA, 
        ONE_MINUS_SRC_ALPHA, 
        DST_ALPHA, 
        ONE_MINUS_DST_ALPHA, 
        CONSTANT_COLOR, 
        ONE_MINUS_CONSTANT_COLOR, 
        CONSTANT_ALPHA, 
        ONE_MINUS_CONSTANT_ALPHA, 
        SRC_ALPHA_SATURATE, 
        SRC1_COLOR, 
        ONE_MINUS_SRC1_COLOR, 
        SRC1_ALPHA, 
        ONE_MINUS_SRC1_ALPHA, 
    };

    enum class EBlendOp : u8
    {
        ADD, 
        SUBTRACT, 
        REVERSE_SUBTRACT, 
        MIN, 
        MAX, 
    };

    enum class EDescriptorType : u8
    {
        SAMPLER, 
        IMAGE_SAMPLER, 
        SAMPLED_IMAGE, 
        STORAGE_IMAGE, 
        UNIFORM_TEXEL_BUFFER, 
        STORAGE_TEXEL_BUFFER, 
        UNIFORM_BUFFER, 
        STORAGE_BUFFER, 
        UNIFORM_DYNAMIC, 
        STORAGE_DYNAMIC, 
    };

    enum class ETextureLayout : u8
    {
        UNDEFINED,
        GENERAL,
        COLOR_ATTACHMENT_OPTIMAL,
        DEPTH_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        SHADER_READ_ONLY_OPTIMAL,
        TRANSFER_SRC_OPTIMAL,
        TRANSFER_DST_OPTIMAL,
    };

    enum EColourMasks : u8
    {
        R = 1 << 0,
        G = 1 << 1,
        B = 1 << 2,
        A = 1 << 3,
    };

    enum EAccessFlags : u8
    {
        COLOR_ATTACHMENT_READ_BIT               = 1 << 0,
        COLOR_ATTACHMENT_WRITE_BIT              = 1 << 1,
        DEPTH_STENCIL_ATTACHMENT_READ_BIT       = 1 << 2,
        DEPTH_STENCIL_ATTACHMENT_WRITE_BIT      = 1 << 3,
        SHADER_WRITE_BIT                        = 1 << 4,
        SHADER_READ_BIT                         = 1 << 5,
    };

    enum EPipelineStageFlags
    {
        VERTEX_INPUT_BIT                        = 1 << 0,
        VERTEX_SHADER_BIT                       = 1 << 1,
        TESSELLATION_CONTROL_SHADER_BIT         = 1 << 2,
        TESSELLATION_EVALUATION_SHADER_BIT      = 1 << 3,
        GEOMETRY_SHADER_BIT                     = 1 << 4,
        FRAGMENT_SHADER_BIT                     = 1 << 5,
        COMPUTE_SHADER_BIT                      = 1 << 6,
        EARLY_FRAGMENT_TESTS_BIT                = 1 << 7,
        LATE_FRAGMENT_TESTS_BIT                 = 1 << 8,
        COLOR_ATTACHMENT_OUTPUT_BIT             = 1 << 9,
        NONE                                    = 1 << 10,
    };

    enum class ERenderingOp : u8
    {
        CLEAR,
        // STORE,
        LOAD
    };

    struct Rect2DInt
    {
        i16 x{ 0 };
        i16 y{ 0 };
        f32 width{ 0.0f };
        f32 height{ 0.0f };
    };

    struct Viewport
    {
        Rect2DInt rect;
        f32 minDepth{ 0.0f };
        f32 maxDepth{ 1.0f };
    };

    struct ViewportStateDesc
    {
        u32 numViewports{ 0 };
        u32 numScissors{ 0 };

        Viewport* viewport{ nullptr };
        Rect2DInt* scissors{ nullptr };
    };

    struct StencilOpState
    {
        EStencilOp fail{ EStencilOp::KEEP };
        EStencilOp pass{ EStencilOp::KEEP };
        EStencilOp depthFail{ EStencilOp::KEEP };
        EComparisonFunc comapare{ EComparisonFunc::ALWAYS };

        u32 compareMask{ 0xff };
        u32 writeMask{ 0xff };
        u32 reference{ 0xff };
    };

    struct DepthStencilDesc
    {
        StencilOpState front;
        StencilOpState back;
        EComparisonFunc depthComparison{ EComparisonFunc::ALWAYS };
        bool depthEnable{ true };
        bool depthWriteEnable{ true };
        bool stencilEnable{ true };
    };

    struct BlendStateDesc
    {
        EBlendState srcColour{ EBlendState::ONE };
        EBlendState dstColour{ EBlendState::ONE };
        EBlendOp colourOp{ EBlendOp::ADD };

        EBlendState srcAlpha{ EBlendState::ONE };
        EBlendState dstAlpha{ EBlendState::ONE };
        EBlendOp alphaOp{ EBlendOp::ADD };

        u8 colourWriteMask{ EColourMasks::R | EColourMasks::G | EColourMasks::B | EColourMasks::A };

        bool blendEnabled{ false };
        bool seperateBlend{ false };
    };

    struct Binding
    {
        EDescriptorType type{ EDescriptorType::UNIFORM_BUFFER };
        EShaderStage stage{ EShaderStage::VERTEX_STAGE };
        u32 count{ 0 };
        u32 bind{ 0 };
    };

    struct DescriptorSetLayoutDesc
    {
        Binding bindings[MAX_DESCRIPTORS];
        u32 numBindings{ 0 };
        u32 setIndex{ 0 };
    };

    struct TextureDesc
    {
        ETextureType type{ ETextureType::TEXTURE2D };
        ETextureFormat format{ ETextureFormat::UNDEFINED };
        u32 width{ 0 };
        u32 height{ 0 };
        u32 depth{ 0 };
        bool isRenderTarget{ false };
        bool isStorageImage{ false };
        bool isMipmapped{ false };
        bool isSampledImage{ true };
    };

    struct BufferDesc
    {
        u8 type{ EBufferType::INVALID };
        EMemoryType memoryType{ EMemoryType::DEVICE_LOCAL };
        u64 bufferSize{ 0 };
    };

    struct SamplerDesc
    {
        ESamplerFilterMode minFilter{ ESamplerFilterMode::NEAREST };
        ESamplerFilterMode magFilter{ ESamplerFilterMode::NEAREST };
        ESamplerMipMapMode mipmap{ ESamplerMipMapMode::NEAREST };
        ESamplerAddressMode u{ ESamplerAddressMode::REPEAT };
        ESamplerAddressMode v{ ESamplerAddressMode::REPEAT };
        ESamplerAddressMode w{ ESamplerAddressMode::REPEAT };
        f32 minLod{ 0.0f };
        f32 maxLod{ LOD_CLAMP_NONE };
    };

    struct ShaderDesc
    {
        EShaderStage stage{ EShaderStage::VERTEX_STAGE };
        cstring shaderName{ nullptr };
    };

    struct ShaderStateDesc
    {
        ShaderDesc shaders[MAX_SHADER_STAGES];
        u32 numStages{ 0 };
    };

    struct RasterizationDesc
    {
        ECullMode cullMode{ ECullMode::NONE };
        EFrontFace frontFace{ EFrontFace::COUNTER_CLOCKWISE };
        EFillMode fillMode{ EFillMode::SOLID };
    };

    struct PushConstantDesc
    {
        u32 offset{ 0 };
        u32 size{ 0 };
        u8 stage{ EShaderStage::INVALID_SHADER_STAGE };
    };

    struct GraphicsPipelineDesc
    {
        RasterizationDesc rDesc;
        DepthStencilDesc dsDesc;
        BlendStateDesc bsDesc[MAX_COLOUR_ATTACHMENTS]{};
        ShaderStateDesc sDesc;
        ViewportStateDesc vpDesc;
        DescriptorSetLayoutDesc layoutDesc;
        PushConstantDesc pushConstant;
        EPrimitiveTopology topology{ EPrimitiveTopology::TRIANGLE };
        TextureHandle* imageAttachments{ nullptr };
        u32 numImageAttachments{ 0 };
        TextureHandle* depthAttachment{ nullptr };

        cstring name{ nullptr };
    };

    struct ComputePipelineDesc
    {
        ShaderDesc computeShader;
        DescriptorSetLayoutDesc layoutDesc;
        PushConstantDesc pushConstant;
        TextureHandle* imageAttachments{ nullptr };
        u32 numImageAttachments{ 0 };
        bool bUseDepthBuffer{ false };

        cstring name{ nullptr };
    };

    #pragma pack(push, 1)
    struct IndirectDraw
    {
        u32 indexCount;
        u32 instanceCount;
        u32 firstIndex;
        i32 vertexOffset;
        u32 firstInstance;
    };
    #pragma pack(pop)
}
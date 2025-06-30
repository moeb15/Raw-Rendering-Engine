#pragma once

#include "renderer/gpu_resources.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Raw::GFX
{
    // must be 256 byte aligned
    struct GlobalSceneData
    {
        alignas(16) glm::mat4 view{ 1.f };
        alignas(16) glm::mat4 projection{ 1.f };
        alignas(16) glm::mat4 viewProj{ 1.f };
        alignas(16) glm::mat4 lightView{ glm::lookAt(glm::vec3(-0.5f, -1.0f, 0.0f), glm::vec3(0), glm::vec3(0, 1, 0)) };
        alignas(16) glm::mat4 lightProj{ glm::ortho(-20.f, 20.f, -20.f, 20.f, -20.f, 20.f) };
        alignas(16) glm::vec4 lightDir{ -0.5f, -1.f, 0.f, 0.f };
        f32 lightIntensity{ 10.f };
        u32 shadowMapIndex{ U32_MAX };
        f32 padding0[2];
        glm::mat4 padding1[2];
        glm::vec4 padding2[2];
    };

    // 16 byte aligned
    struct VertexData
    {
        glm::vec3 position;
        f32 texCoordU;
        glm::vec3 normal;
        f32 texCoordV;
    };

    struct PBRMaterialData
    {
        i32 diffuse{ -1 };
        i32 roughness{ -1 };
        i32 normal{ -1 };
        i32 occlusion{ -1 };
        i32 emissive{ -1 };
        glm::vec4 baseColorFactor{ 1.f, 1.f, 1.f, 1.f };
        glm::vec2 metalRoughnessFactor{ 1.f, 1.f };
        f32 alphaCutoff{ 1.f };
        bool isTransparent{ false };
    };

    struct MeshData
    {
        u64 meshId;
        u32 indexCount;
        u32 firstIndex;
        u32 materialIndex;
        glm::vec3 rotation{ 0.f };
        glm::vec3 translation{ 0.f };
        glm::vec3 scale{ 1.f };
    };

    struct NodeData
    {
        u64 vertexId;
        u64 indexId;
        glm::mat4 transform;
        std::vector<MeshData> meshes;
    };

    struct SceneData
    {
        std::vector<NodeData> nodes;
        std::vector<u32> images;
        std::vector<u32> textures;
        std::vector<PBRMaterialData> materials;
    };
}
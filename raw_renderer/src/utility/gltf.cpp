#include "utility/gltf.hpp"
#include "utility/hash.hpp"
#include "core/servicelocator.hpp"
#include "core/asserts.hpp"
#include "renderer/gfxdevice.hpp"
#include "resources/texture_loader.hpp"
#include "resources/buffer_loader.hpp"
#include "core/timer.hpp"
#include "core/job_system.hpp"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>

using namespace tinygltf;

namespace Raw::Utils
{
    namespace
    {
        void LoadNode(const Node& inputNode, Model& input, GFX::SceneData& outSceneData, glm::mat4 parentTransform, std::vector<GFX::VertexData>& vertices, std::vector<u32>& indices)
        {
            glm::mat4 curTransform = glm::mat4(1.f);

            if(inputNode.translation.size() == 3)
            {
                curTransform = glm::translate(curTransform, glm::vec3(glm::make_vec3(inputNode.translation.data())));
            }
            if(inputNode.rotation.size() == 4)
            {
                glm::quat q = glm::make_quat(inputNode.rotation.data());
                curTransform *= glm::mat4(q);
            }
            if(inputNode.scale.size() == 3)
            {
                curTransform = glm::scale(curTransform, glm::vec3(glm::make_vec3(inputNode.scale.data())));
            }
            if(inputNode.matrix.size() == 16)
            {
                curTransform = glm::make_mat4x4(inputNode.matrix.data());
            }

            curTransform *= parentTransform;

            outSceneData.transforms.push_back(curTransform);
            
            /*
            if(inputNode.children.size() > 0)
            {
                for(u64 i = 0; i < inputNode.children.size(); i++)
                {
                    const Node& childNode = input.nodes[inputNode.children[i]];
                    LoadNode(childNode, input, outSceneData, curTransform, vertices, indices);
                }
            }*/
            
            if(inputNode.mesh > -1)
            {
                const Mesh gltfMesh = input.meshes[inputNode.mesh];
                RAW_DEBUG("Loading glTF mesh: %s", gltfMesh.name.c_str());
                GFX::MeshData mesh;
                
                for(u64 i = 0; i < gltfMesh.primitives.size(); i++)
                {
                    GFX::MeshData mesh;
                    u64 meshId = Utils::HashCString(gltfMesh.name.c_str());
                    const Primitive& gltfPrimitive = gltfMesh.primitives[i];
                    u32 firstIndex = indices.size();
                    u32 vertexStart = vertices.size();
                    u32 indexCount = 0;

                    // vertices
                    {
                        const f32* positionBuffer = nullptr;
                        const f32* normalBuffer = nullptr;
                        const f32* texCoordBuffer = nullptr;
                        u64 vertexCount = 0;

                        if(gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end())
                        {
                            const Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("POSITION")->second];
                            const BufferView& view = input.bufferViews[accessor.bufferView];
                            positionBuffer = reinterpret_cast<const f32*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                            vertexCount = accessor.count;
                        }
                        
                        if(gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end())
                        {
                            const Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("NORMAL")->second];
                            const BufferView& view = input.bufferViews[accessor.bufferView];
                            normalBuffer = reinterpret_cast<const f32*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        }

                        if(gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end())
                        {
                            const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("TEXCOORD_0")->second];
                            const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                            texCoordBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        }

                        for(u64 v = 0; v < vertexCount; v++)
                        {
                            GFX::VertexData vert{};
                            vert.position = glm::make_vec3(&positionBuffer[v*3]);
                            vert.normal = glm::normalize(glm::vec3(normalBuffer ? glm::make_vec3(&normalBuffer[v * 3]) : glm::vec3(0.0f)));
                            glm::vec2 uv = texCoordBuffer ? glm::make_vec2(&texCoordBuffer[v * 2]) : glm::vec2(0.0f);
                            vert.texCoordU = uv.x;
                            vert.texCoordV = uv.y;

                            vertices.push_back(vert);
                        }
                    }

                    // indices
                    {
                        const Accessor& accessor = input.accessors[gltfPrimitive.indices];
                        const BufferView& bufferView = input.bufferViews[accessor.bufferView];
                        const Buffer& buffer = input.buffers[bufferView.buffer];

                        indexCount += (u32)accessor.count;
                        switch(accessor.componentType)
                        {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            {
                                const u32* buf = reinterpret_cast<const u32*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                                for(u64 index = 0; index < accessor.count; index++)
                                {
                                    indices.push_back(buf[index] + vertexStart);
                                }
                                break;
                            }
                            
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            {
                                const u16* buf = reinterpret_cast<const u16*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                                for(u64 index = 0; index < accessor.count; index++)
                                {
                                    indices.push_back(buf[index] + vertexStart);
                                }
                                break;
                            }

                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            {
                                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                                for(u64 index = 0; index < accessor.count; index++)
                                {
                                    indices.push_back(buf[index] + vertexStart);
                                }
                                break;
                            }
                            default:
                                RAW_ERROR("Index of component type %u not supported!", accessor.componentType);
                                return;
                        }
                    }

                    mesh.firstIndex = firstIndex;
                    mesh.indexCount = indexCount;
                    mesh.materialIndex = gltfPrimitive.material;
                    mesh.vertexOffset = 0;
                    mesh.baseInstance = 0;
                    mesh.instanceCount = 1;
                    mesh.transformIndex = (u32)outSceneData.transforms.size() - 1;

                    outSceneData.meshes.push_back(mesh);
                }
            }
        }
        void LoadImages(Model& input, std::vector<u32>& images)
        {
            for(u64 i = 0; i < input.images.size(); i++)
            {
                Image& glTFImage = input.images[i];
                RAW_DEBUG("Loading glTF image: %s", glTFImage.name.c_str());
                u8* buffer = nullptr;
                u64 bufferSize = 0;

                if(glTFImage.component == 3)
                {
                    bufferSize = glTFImage.width * glTFImage.height * 4;
                    buffer = (u8*)malloc(bufferSize);
                    u8* rgba = buffer;
                    u8* rgb = &glTFImage.image[0];
                    for(u64 j = 0; j < glTFImage.width * glTFImage.height; i++)
                    {
                        memcpy(rgba, rgb, sizeof(u8) * 3);
                        rgba += 4;
                        rgb += 3;
                    }

                    GFX::TextureDesc desc;
                    desc.depth = 1;
                    desc.width = glTFImage.width;
                    desc.height = glTFImage.height;
                    desc.isMipmapped = true;
                    desc.isRenderTarget = false;
                    desc.isStorageImage = false;
                    desc.type = GFX::ETextureType::TEXTURE2D;
                    desc.format = GFX::ETextureFormat::R8G8B8A8_UNORM;

                    TextureResource* tex = (TextureResource*)TextureLoader::Instance()->CreateFromData(glTFImage.uri.c_str(), desc, buffer);
                    images.push_back(tex->handle.id);

                    free(buffer);
                }
                else
                {
                    buffer = &glTFImage.image[0];
                    bufferSize = glTFImage.image.size();

                    GFX::TextureDesc desc;
                    desc.depth = 1;
                    desc.width = glTFImage.width;
                    desc.height = glTFImage.height;
                    desc.isMipmapped = true;
                    desc.isRenderTarget = false;
                    desc.isStorageImage = false;
                    desc.type = GFX::ETextureType::TEXTURE2D;
                    desc.format = GFX::ETextureFormat::R8G8B8A8_UNORM;

                    TextureResource* tex = (TextureResource*)TextureLoader::Instance()->CreateFromData(glTFImage.uri.c_str(), desc, buffer);
                    if(tex) images.push_back(tex->handle.id);
                }
            }
        }

        void LoadTextures(Model& input, std::vector<u32>& textures)
        {
            for(u64 i = 0; i < input.textures.size(); i++)
            {
                RAW_DEBUG("Loading glTF texture: %u", input.textures[i].source);
                textures.push_back(input.textures[i].source);
            }
        }

        void LoadMaterials(Model& input, std::vector<GFX::PBRMaterialData>& materials)
        {
            for(u64 i = 0; i < input.materials.size(); i++)
            {
                RAW_DEBUG("Loading glTF material: %llu", i);
                GFX::PBRMaterialData materialData = GFX::PBRMaterialData();

                Material glTFMaterial = input.materials[i];
                if(glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())                materialData.baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
                if(glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())               materialData.diffuse = glTFMaterial.values["baseColorTexture"].TextureIndex();
                
                materialData.roughness = glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                materialData.normal = glTFMaterial.normalTexture.index;
                materialData.occlusion = glTFMaterial.occlusionTexture.index;
                materialData.metalRoughnessFactor.x = glTFMaterial.pbrMetallicRoughness.metallicFactor;
                materialData.metalRoughnessFactor.y = glTFMaterial.pbrMetallicRoughness.roughnessFactor;
                materialData.emissive = glTFMaterial.emissiveTexture.index;
                materialData.alphaCutoff = glTFMaterial.alphaCutoff;
                materialData.isTransparent = glTFMaterial.alphaMode == "OPAQUE" ? false : true;

                materials.push_back(materialData);
            }
        }
    }

    void LoadGLTF(std::string filepath, GFX::SceneData& outScene)
    {
        Model model;
        TinyGLTF loader;
        std::string err;
        std::string warn;

        bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

        if(!warn.empty())   RAW_WARN("GLTF WARNING: %s", warn.c_str());
        if(!err.empty())    RAW_ERROR("GLTF ERROR: %s", err.c_str());

        RAW_ASSERT_MSG(ret, "Failed to parse glTF '%s'", filepath.c_str());
        

        u64 startTime = Timer::Get()->Now();

        JobSystem::Execute([&](){ LoadImages(model, outScene.images); });
        JobSystem::Execute([&](){ LoadTextures(model, outScene.textures); });
        JobSystem::Execute([&](){ LoadMaterials(model, outScene.materials);});

        
        JobSystem::Execute([&]()
            {
                std::vector<GFX::VertexData> vertices;
                std::vector<u32> indices;

                for(u64 i = 0; i < model.nodes.size(); i++)
                {
                    Node& curNode = model.nodes[i];
                    LoadNode(curNode, model, outScene, glm::mat4(1.f), vertices, indices);
                }

                GFX::BufferDesc vertexDesc;
                vertexDesc.bufferSize = vertices.size() * sizeof(GFX::VertexData);
                vertexDesc.memoryType = GFX::EMemoryType::DEVICE_LOCAL;
                vertexDesc.type = GFX::EBufferType::VERTEX;
                
                GFX::BufferDesc indexDesc;
                indexDesc.bufferSize = indices.size() * sizeof(u32);
                indexDesc.memoryType = GFX::EMemoryType::DEVICE_LOCAL;
                indexDesc.type = GFX::EBufferType::INDEX;

                std::string vBufferName = filepath + "_vertex";
                std::string iBufferName = filepath + "_index";
                BufferResource vRes = *(BufferResource*)BufferLoader::Instance()->CreateBuffer(vBufferName.c_str(), vertexDesc, vertices.data());
                BufferResource iRes = *(BufferResource*)BufferLoader::Instance()->CreateBuffer(iBufferName.c_str(), indexDesc, indices.data());
                
                outScene.vertexBuffer = vRes.buffer;
                outScene.indexBuffer = iRes.buffer;
            }
        );

        JobSystem::Wait();
        u64 endTime = Timer::Get()->Now();
        f64 deltaTime = Timer::Get()->DeltaSeconds(startTime, endTime);

        RAW_TRACE("GLTF file '%s' loaded.", filepath.c_str());
        RAW_TRACE("Load Time: %0.2llf s", deltaTime);
    }
}
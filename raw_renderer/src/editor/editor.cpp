#include "editor/editor.hpp"
#include "renderer/renderer_data.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>

namespace Raw
{
    static Editor s_Editor;

    Editor* Editor::Get()
    {
        return &s_Editor;
    }
    
    void ProcessEvent(SDL_Event* event)
    {
        ImGui_ImplSDL3_ProcessEvent(event);
    }
    
    void Editor::Render(f32 dt, GFX::SceneData& sceneData, GFX::GlobalSceneData& globalData)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Begin("Statistics");
        ImGui::Separator();
        ImGui::Text("Framerate: %u fps", (u32)io.Framerate);
        ImGui::Text("Frametime: %0.2f ms", dt * 1000.f);
        ImGui::End();

        ImGui::Begin("Global Scene Data");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Directional Light Settings:");
        ImGui::SliderFloat("X", &globalData.lightDir.x, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Y", &globalData.lightDir.y, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Z", &globalData.lightDir.z, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Intensity", &globalData.lightIntensity, 0.f, 100.f, "%.1f");
        ImGui::End();
        globalData.lightDir = glm::normalize(globalData.lightDir);
        globalData.lightView = glm::lookAt(glm::vec3(0, 15.f, 0), glm::vec3(-globalData.lightDir.x, -globalData.lightDir.y, -globalData.lightDir.z), glm::vec3(0, 0, 1));
        globalData.lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, -15.f, 15.f);
    
        ImGui::Begin("Meshes");
        for(u32 i = 0; i < sceneData.meshes.size(); i++)
        {
            if(ImGui::TreeNode(&sceneData.meshes[i], "Mesh %u", i))
            {
                GFX::MeshData& mesh = sceneData.meshes[i];
                
                {
                    GFX::PBRMaterialData& materialData = sceneData.materials[mesh.materialIndex];                    
                    ImGui::Text("Material Data");
                    ImGui::Spacing();
                    ImGui::Text("Base Color Factor");
                    ImGui::SliderFloat("R", &materialData.baseColorFactor.x, 0.0f, 1.0f);
                    ImGui::SliderFloat("G", &materialData.baseColorFactor.y, 0.0f, 1.0f);
                    ImGui::SliderFloat("B", &materialData.baseColorFactor.z, 0.0f, 1.0f);
                    ImGui::SliderFloat("A", &materialData.baseColorFactor.w, 0.0f, 1.0f);
                    ImGui::Spacing();
                    ImGui::Text("Metallic Roughness Factor");
                    ImGui::SliderFloat("Metallic", &materialData.metalRoughnessFactor.x, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", &materialData.metalRoughnessFactor.y, 0.0f, 1.0f);
                }
                
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
}
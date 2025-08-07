#include "editor/editor.hpp"
#include "renderer/renderer.hpp"
#include "events/renderer_events.hpp"
#include "events/event_manager.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>

namespace Raw
{
    static Editor s_Editor;
    static GFX::RenderPassData prevData;
    static i32 lightIndex = 0;

    Editor* Editor::Get()
    {
        return &s_Editor;
    }
    
    void ProcessEvent(SDL_Event* event)
    {
        ImGui_ImplSDL3_ProcessEvent(event);
    }
    
    void Editor::Render(f32 dt, Scene* scene, GFX::GlobalSceneData& globalData, GFX::RenderPassData* passData)
    {
        GFX::SceneData& sceneData = *scene->GetSceneData();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Begin("Editor");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("W : Move Forward");
        ImGui::Text("S : Move Backward");
        ImGui::Text("A : Move Left");
        ImGui::Text("D : Move Right");
        ImGui::Text("Q : Move Up");
        ImGui::Text("E : Move Down");
        ImGui::Text("RMB : Rotate Camera");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Statistics");
        ImGui::Text("Framerate: %u fps", (u32)io.Framerate);
        ImGui::Text("Frametime: %0.2f ms", dt * 1000.f);
        ImGui::Spacing();
        
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Render Pass Data");
        ImGui::Checkbox("SSAO", &passData->enableAO);
        ImGui::Checkbox("SSR", &passData->enableSSR);
        ImGui::Checkbox("FXAA", &passData->enableFXAA);
        ImGui::Spacing();
        
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Directional Light Settings:");
        ImGui::SliderFloat("X", &globalData.lightDir.x, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Y", &globalData.lightDir.y, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Z", &globalData.lightDir.z, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("Intensity", &globalData.lightIntensity, 0.f, 100.f, "%.1f");
        globalData.lightDir = glm::normalize(globalData.lightDir);
        globalData.lightView = glm::lookAt(glm::vec3(0, 15.f, 0), glm::vec3(-globalData.lightDir.x, -globalData.lightDir.y, -globalData.lightDir.z), glm::vec3(0, 0, 1));
        globalData.lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, -15.f, 15.f);
    
        ImGui::Spacing();
        ImGui::Text("Point Lights");
        ImGui::SliderInt("Point Light Index", &lightIndex, 0, MAX_LIGHT_COUNT - 1);
        GFX::PointLight* rootLight = scene->GetPointLights();

        ImGui::Text("Position:");
        ImGui::SliderFloat("PointLight X", &rootLight[lightIndex].position.x, -10.f, 10.f, "%.3f");
        ImGui::SliderFloat("PointLight Y", &rootLight[lightIndex].position.y, -10.f, 10.f, "%.3f");
        ImGui::SliderFloat("PointLight Z", &rootLight[lightIndex].position.z, -10.f, 10.f, "%.3f");

        ImGui::Text("Direction:");
        ImGui::SliderFloat("PointLight Dir X", &rootLight[lightIndex].direction.x, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("PointLight Dir Y", &rootLight[lightIndex].direction.y, -1.f, 1.f, "%.3f");
        ImGui::SliderFloat("PointLight Dir Z", &rootLight[lightIndex].direction.z, -1.f, 1.f, "%.3f");

        ImGui::Text("Color:");
        ImGui::SliderFloat("R", &rootLight[lightIndex].color.r, 0.f, 1.f, "%.1f");
        ImGui::SliderFloat("G", &rootLight[lightIndex].color.g, 0.f, 1.f, "%.1f");
        ImGui::SliderFloat("B", &rootLight[lightIndex].color.b, 0.f, 1.f, "%.1f");
        ImGui::SliderFloat("A", &rootLight[lightIndex].color.a, 0.f, 1.f, "%.1f");

        ImGui::SliderFloat("Intensity:", &rootLight[lightIndex].intensity, 0.f, 10.f, "%.1f");
        ImGui::SliderFloat("Radius:", &rootLight[lightIndex].radius, 0.f, 100.f, "%.1f");

        rootLight[lightIndex].direction = glm::normalize(rootLight[lightIndex].direction);

        ImGui::Spacing();
        ImGui::Text("Meshes");
        if(ImGui::TreeNode("Root"))
        {
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
            ImGui::TreePop();
        }
        ImGui::End();

        if(prevData.enableAO != passData->enableAO)         EventManager::Get()->TriggerEvent(rstd::make_unique<AOToggledEvent>(passData->enableAO));
        if(prevData.enableSSR != passData->enableSSR)       EventManager::Get()->TriggerEvent(rstd::make_unique<ReflectionsToggledEvent>(passData->enableSSR));
        if(prevData.enableFXAA != passData->enableFXAA)     EventManager::Get()->TriggerEvent(rstd::make_unique<AntiAliasingToggledEvent>(passData->enableSSR));

        prevData = *passData;
    }
}
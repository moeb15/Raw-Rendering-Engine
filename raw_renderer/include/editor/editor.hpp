#pragma once

#include "core/defines.hpp"
#include "renderer/renderer_data.hpp"
#include "scene/scene.hpp"

union SDL_Event;

namespace Raw
{
    namespace GFX
    {
        struct RenderPassData;
    }

    class Editor
    {
    public:
        static Editor* Get();
        void ProcessEvent(SDL_Event* event);
        void Render(f32 dt, Scene* scene, GFX::GlobalSceneData& globalData, GFX::RenderPassData* passData);
    };
}
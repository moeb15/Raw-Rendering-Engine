#pragma once

#include "renderer/renderer_data.hpp"
#include <string>

namespace Raw::Utils
{
   void LoadGLTF(std::string filepath, GFX::SceneData& outSceneData);
}
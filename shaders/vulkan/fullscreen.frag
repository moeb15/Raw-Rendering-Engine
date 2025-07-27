#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];
layout(set = 1, binding = 0) uniform sampler3D globalTextures3D[];

layout(set = 1, binding = 1) uniform texture2D globalImages[];
layout(set = 1, binding = 2) uniform sampler shadowSampler;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants{
    uint sceneTexture;
} PushConstants;


void main() 
{
    outFragColor = texture(globalTextures[(PushConstants.sceneTexture)], inUV);
}
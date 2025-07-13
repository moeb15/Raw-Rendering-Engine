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
    uint diffuse;
    uint roughness;
    uint normal;
    uint emissive;
    uint viewspace;
    uint transparent;
} PushConstants;

#define PI 3.1415926538
#define AMBIENT 0.5
#define BIAS 0.0005

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

float distGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return (a2 * heaviside(NdotH)) / max(denom, 0.0001);
}

float geomSchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float denom = NdotV * (1.0 - k) + k;
    
    return NdotV / denom;
}

float geomSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geomSchlickGGX(NdotV, roughness);
    float ggx2 = geomSchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) *pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() 
{
    vec4 baseColor = texture(globalTextures[nonuniformEXT(PushConstants.diffuse)], inUV);
    vec3 normal = texture(globalTextures[nonuniformEXT(PushConstants.normal)], inUV).rgb;
	vec4 rmOcc = texture(globalTextures[nonuniformEXT(PushConstants.roughness)], inUV);
	vec4 emissive = texture(globalTextures[nonuniformEXT(PushConstants.emissive)], inUV);
    vec4 viewPos = texture(globalTextures[nonuniformEXT(PushConstants.viewspace)], inUV);
    vec4 transparency = texture(globalTextures[nonuniformEXT(PushConstants.transparent)], inUV);

    mat3 rot = mat3(GlobalSceneData.view);
    vec3 translation = vec3(GlobalSceneData.view[3]);
    vec3 eyePos = -translation * rot;

    vec3 V = normalize(vec3(1.0));//normalize(-eyePos.xyz);
    vec3 L = normalize(-GlobalSceneData.lightDir.xyz);
    vec3 N = normal;
    vec3 H = normalize(L + V);

	float roughness = rmOcc.r;
    float metalness = rmOcc.g;
	float occlusion = rmOcc.b;
	float alpha = pow(roughness, 2.0);

    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metalness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float NDF = distGGX(N, H, metalness);
    float G = geomSmith(N, V, L, metalness);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 diffuse = baseColor.rgb / PI;

    float NdotL = clamp(max(dot(N, L), 0.0), 0, 1);
    vec3 radiance = vec3(1.0) * GlobalSceneData.lightIntensity;

    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    vec3 ambient = baseColor.rgb * AMBIENT * occlusion;
    vec3 color = emissive.rgb + ambient + Lo;

    if(transparency.a > 0.0 && length(transparency.rgb) > 0.001)
    {
        color = mix(color, transparency.rgb, transparency.a);
    }
    outFragColor = vec4(color, 1);
}
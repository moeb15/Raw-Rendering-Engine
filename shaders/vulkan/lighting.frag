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
    uint lightClipSpacePos;
    uint occlusion;
    uint transparent;
    uint reflection;
} PushConstants;

float textureProj(vec3 projCoords, vec2 off)
{
	float closestDepth = texture(sampler2D(globalImages[GlobalSceneData.shadowMapIndex], shadowSampler), projCoords.xy + off).r;
	float curDepth = projCoords.z;

	float shadow = curDepth - BIAS > closestDepth ? 1.0 : 0.0;
	return shadow;
}

float filterPCF(vec4 shadowPos)
{
	ivec2 texDim = textureSize(sampler2D(globalImages[GlobalSceneData.shadowMapIndex], shadowSampler), 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	vec3 projCoords = shadowPos.xyz / shadowPos.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
	{
		return 0.0;
	}

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for(int x = -range; x <= range; x++)
	{
		for(int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(projCoords, vec2(dx * x, dy * y));
			count++;
		}
	}
	return shadowFactor / count;
}

void main() 
{
    vec4 baseColor = texture(globalTextures[nonuniformEXT(PushConstants.diffuse)], inUV);
    vec4 reflectColor = texture(globalTextures[nonuniformEXT(PushConstants.reflection)], inUV);
    vec3 normal = texture(globalTextures[nonuniformEXT(PushConstants.normal)], inUV).rgb;
	vec4 rmOcc = texture(globalTextures[nonuniformEXT(PushConstants.roughness)], inUV);
	vec4 emissive = texture(globalTextures[nonuniformEXT(PushConstants.emissive)], inUV);
    vec4 viewPos = texture(globalTextures[nonuniformEXT(PushConstants.viewspace)], inUV);
    vec4 transparency = texture(globalTextures[nonuniformEXT(PushConstants.transparent)], inUV);
    vec4 occlusionTex = texture(globalTextures[nonuniformEXT(PushConstants.occlusion)], inUV);

    mat3 rot = mat3(GlobalSceneData.view);
    vec3 translation = vec3(GlobalSceneData.view[3]);
    vec3 eyePos = -translation * rot;

    vec3 V = normalize(vec3(1.0));//normalize(-eyePos.xyz);
    vec3 L = normalize(-GlobalSceneData.lightDir.xyz);
    vec3 N = normal;
    vec3 H = normalize(L + V);

	float roughness = rmOcc.r;
    float metalness = rmOcc.g;
	float occlusion = occlusionTex.r;
	float alpha = pow(roughness, 2.0);

    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metalness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float NDF = distGGX(N, H, roughness);
    float G = geomSmith(N, V, L, roughness);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 diffuse = baseColor.rgb / PI;

    float NdotL = clamp(max(dot(N, L), 0.0), 0, 1);
    vec3 radiance = vec3(1.0) * GlobalSceneData.lightIntensity;

    vec4 lightClipSpacePos = texture(globalTextures[nonuniformEXT(PushConstants.lightClipSpacePos)], inUV);
//    ivec2 shadowDim = textureSize(sampler2D(globalImages[GlobalSceneData.shadowMapIndex], shadowSampler), 0);
//    ivec2 backBufferDim = textureSize(globalTextures[nonuniformEXT(PushConstants.diffuse)], 0);

    //vec4 shadowUV;
    // shadowUV.xy = lightClipSpacePos.xy * (shadowDim / backBufferDim);
    // shadowUV.z = lightClipSpacePos.z;
    // shadowUV.w = lightClipSpacePos.w;

    float shadow = filterPCF(lightClipSpacePos);    
    vec3 relfectedLight = mix(specular, reflectColor.rgb, reflectColor.a);

    vec3 Lo = (diffuse + relfectedLight) * radiance * NdotL;
    vec3 ambient = baseColor.rgb * AMBIENT;
    vec3 color = emissive.rgb + ambient + Lo;

    if(transparency.a > 0.0 && length(transparency.rgb) > 0.001)
    {
        color = mix(color, transparency.rgb, transparency.a);
    }
    color *= occlusion;
    outFragColor = vec4(color, 1.0);
}
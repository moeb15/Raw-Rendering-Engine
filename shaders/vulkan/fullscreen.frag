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
    uint occlusion;
    uint emissive;
    uint viewspace;
    uint transparent;
} PushConstants;

#define PI 3.1415926538
#define AMBIENT 0.1
#define BIAS 0.0005

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

void main() 
{
    vec4 baseColor = texture(globalTextures[nonuniformEXT(PushConstants.diffuse)], inUV);
    outFragColor = baseColor;
/*
    vec3 normal = normalize(texture(globalTextures[nonuniformEXT(PushConstants.normal)], inUV).rgb);
	vec4 rm = texture(globalTextures[nonuniformEXT(PushConstants.roughness)], inUV);
	vec4 occ = texture(globalTextures[nonuniformEXT(PushConstants.occlusion)], inUV);
	vec4 emissive = texture(globalTextures[nonuniformEXT(PushConstants.emissive)], inUV);

    mat3 rot = mat3(GlobalSceneData.view);
    vec3 translation = vec3(GlobalSceneData.view[3]);
    vec3 eyePos = -translation * rot;

    vec3 V = normalize(eyePos.xyz);
    vec3 L = normalize(-GlobalSceneData.lightDir.xyz);
    vec3 N = normal;
    vec3 H = normalize(L + V);

	float metalness = rm.g;
	float roughness = rm.b;
	float alpha = pow(roughness, 2.0);
	float occlusion = occ.r;

	float NdotH = clamp(dot(N, H), 0, 1);
	float alpahSqr = alpha * alpha;
	float dDenom = (NdotH * NdotH) * (alpahSqr - 1.0) + 1.0;
	float distribution = (alpahSqr) / (PI * dDenom * dDenom);

	float NdotL = clamp(dot(N, L), 0, 1);
	float NdotV = clamp(dot(N, V), 0, 1);
	float HdotL = clamp(dot(H, L), 0, 1);
	float HdotV = clamp(dot(H, V), 0, 1);

	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
	float gl = NdotL / (NdotL * (1.0 - k ) + k);
	float gv = NdotV / (NdotV * (1.0 - k ) + k);

    float visibility = gl * gv;
	vec3 f0 = mix(vec3(0.04), baseColor.rgb, metalness);
	vec3 fr = f0 + ( 1 - f0 ) * pow(1 - abs( HdotV ), 5 );
	
    vec3 specularBRDF =  fr * visibility * distribution / (4.0 * NdotV *NdotL + 0.001);

	vec3 kd = (1.0 - fr) * ( 1.0 - metalness );
	vec3 diffuseBRDF = kd * (1 / PI) * baseColor.rgb;

    vec3 materialColor = vec3(0,0,0);
	vec3 ambient = baseColor.rgb * AMBIENT * occlusion;

    vec3 directLight = (ambient + diffuseBRDF + specularBRDF) * NdotL * GlobalSceneData.lightIntensity;
	materialColor = emissive.rgb * 0.5 + ambient + directLight;

    vec4 transparent = texture(globalTextures[nonuniformEXT(PushConstants.transparent)], inUV);
    if(transparent.r != 0 && transparent.g != 0 && transparent.b != 0)
    {
	    outFragColor = transparent;
    }
    else
    {
	    outFragColor = vec4(materialColor, baseColor.a);
    }*/
}
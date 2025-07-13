#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];
layout(set = 1, binding = 0) uniform sampler3D globalTextures3D[];

layout(set = 1, binding = 1) uniform texture2D globalImages[];
layout(set = 1, binding = 2) uniform sampler shadowSampler;

layout(set = 2, binding = 0) uniform materialData
{
    PBRMaterial materials[MAX_MATERIALS];
} GlobalMaterialData;

//shader input
layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inWorldPos;
layout (location = 2) in vec3 inViewPos;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inTangent;
layout (location = 5) in vec4 inLightPos;
layout (location = 6) in flat uint inMaterialIndex;


//output write
layout (location = 0) out vec4 outFragColor;

#define PI 3.1415926538
#define AMBIENT 0.1
#define BIAS 0.0005

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

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
	if(inMaterialIndex < MAX_MATERIALS)
	{
		PBRMaterial material = GlobalMaterialData.materials[inMaterialIndex];

		vec4 baseColor = texture(globalTextures[nonuniformEXT(material.diffuse)], inUV);
		if(baseColor.a <= material.alphaCutoff)
		{
			discard;
		}
		baseColor.rgba *= material.baseColorFactor.rgba;

		// float gamma = 2.2;
		// baseColor.rgb = pow(baseColor.rgb, vec3(gamma));

		vec3 vNorm = normalize(inNormal);
		vec3 vTan = normalize(inTangent.xyz);
		vec3 bTan = cross(inNormal, inTangent.xyz) * inTangent.w;
		mat3 TBN = mat3(vTan, bTan, vNorm);

		vec3 normal = TBN * normalize(texture(globalTextures[nonuniformEXT(material.normal)], inUV).rgb * 2.0 - 1.0);
		vec4 rm = texture(globalTextures[nonuniformEXT(material.roughness)], inUV);
		vec4 occ = texture(globalTextures[nonuniformEXT(material.occlusion)], inUV);
		vec4 emissive = texture(globalTextures[nonuniformEXT(material.emissive)], inUV);

		mat3 rot = mat3(GlobalSceneData.view);
		vec3 translation = vec3(GlobalSceneData.view[3]);
		vec3 eyePos = -translation * rot;

		vec3 V = normalize(eyePos.xyz);
		vec3 L = normalize(-GlobalSceneData.lightDir.xyz);
		vec3 N = normal;
		vec3 H = normalize(L + V);

		float metalness = rm.g * material.metalRoughnessFactor.x;
		float roughness = rm.b * material.metalRoughnessFactor.y;
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
		
		float shadow = filterPCF(inLightPos);

		vec3 directLight = (ambient + diffuseBRDF + specularBRDF) * NdotL * GlobalSceneData.lightIntensity;
		materialColor = emissive.rgb * 0.5 + ambient + directLight * (1.0 - shadow);

		outFragColor = vec4(materialColor, baseColor.a);
	}
}
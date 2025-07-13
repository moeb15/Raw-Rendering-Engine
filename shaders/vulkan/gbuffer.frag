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
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec4 inViewSpacePos;
layout (location = 4) in vec4 inLightClipSpacePos;
layout (location = 5) in flat uint inMaterialIndex;


//output write
layout (location = 0) out vec4 outDiffuse;
layout (location = 1) out vec4 outNormals;
layout (location = 2) out vec4 outRMOcc;
layout (location = 3) out vec4 outEmissive;
layout (location = 4) out vec4 outViewSpacePos;
layout (location = 5) out vec4 outLightClipSpacePos;


void main() 
{
	if(inMaterialIndex < MAX_MATERIALS)
	{
		PBRMaterial material = GlobalMaterialData.materials[inMaterialIndex];

		vec4 baseColor = texture(globalTextures[nonuniformEXT(material.diffuse)], inUV);
		baseColor.rgba *= material.baseColorFactor.rgba;
		
		vec3 vNorm = normalize(inNormal);
		vec3 vTan = normalize(inTangent.xyz);
		vec3 bTan = cross(inNormal, inTangent.xyz) * inTangent.w;
		mat3 TBN = mat3(vTan, bTan, vNorm);

		vec3 normal = TBN * normalize(texture(globalTextures[nonuniformEXT(material.normal)], inUV).rgb * 2.0 - 1.0);
		vec4 rm = texture(globalTextures[nonuniformEXT(material.roughness)], inUV);
		vec4 occ = texture(globalTextures[nonuniformEXT(material.occlusion)], inUV);
		vec4 emissive = texture(globalTextures[nonuniformEXT(material.emissive)], inUV);

        float metalness = rm.g * material.metalRoughnessFactor.x;
        float roughness = rm.b * material.metalRoughnessFactor.y;
        float alpha = pow(roughness, 2.0);
        float occlusion = occ.r;

        outDiffuse = baseColor;
        outNormals = vec4(normal, 1.0);
        outRMOcc = vec4(roughness, metalness, occlusion, 1.0);
        outEmissive = emissive;
        outViewSpacePos = inViewSpacePos;
		outLightClipSpacePos = inLightClipSpacePos;
	}
}
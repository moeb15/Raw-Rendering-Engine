#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outWorldPos;
layout (location = 2) out vec3 outViewPos;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out vec4 outTangent;
layout (location = 5) out vec4 outLightPos;
layout (location = 6) out uint outMaterialIndex;

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer MeshDrawDataBuffer{
	MeshDrawData meshData[];
};

layout(push_constant) uniform constants{
	VertexBuffer vertexBuffer;
	MeshDrawDataBuffer meshBuffer;
} PushConstants;

void main() 
{	
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	MeshDrawData drawData = PushConstants.meshBuffer.meshData[gl_DrawID];
	if(drawData.isTransparent == 0)
	{
		return;
	}

	//output the position of each vertex
	gl_Position = GlobalSceneData.viewProj * drawData.transform * vec4(v.position, 1.0f);
	outUV = vec2(v.u, v.v);
	outWorldPos = drawData.transform * vec4(v.position, 1.0f);
	outViewPos = outWorldPos.xyz / outWorldPos.w;
	outNormal = v.normal;
	outLightPos = GlobalSceneData.lightProj * GlobalSceneData.lightView * drawData.transform * vec4(v.position, 1.0f);
	outMaterialIndex = drawData.materialIndex;
	outTangent = v.tangent;
}
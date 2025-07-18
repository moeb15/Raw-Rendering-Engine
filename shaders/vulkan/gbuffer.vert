#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outTangent;
layout (location = 3) out vec4 outViewSpacePos;
layout (location = 4) out vec4 outLightClipSpacePos;
layout (location = 5) out uint outMaterialIndex;

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
	if(drawData.isTransparent == 1)
	{
		return;
	}

	//output the position of each vertex
	gl_Position = GlobalSceneData.viewProj * drawData.transform * vec4(v.position, 1.0f);
	outUV = vec2(v.u, v.v);
	outNormal = mat3(drawData.transform) * v.normal;
	outMaterialIndex = drawData.materialIndex;
	outTangent = v.tangent;
    outViewSpacePos = GlobalSceneData.view * drawData.transform * vec4(v.position, 1.0f);
	outLightClipSpacePos = GlobalSceneData.lightProj * GlobalSceneData.lightView * drawData.transform * vec4(v.position, 1.0f);
}
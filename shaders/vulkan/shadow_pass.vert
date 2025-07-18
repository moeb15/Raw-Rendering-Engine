#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

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

	gl_Position = GlobalSceneData.lightProj * GlobalSceneData.lightView * drawData.transform * vec4(v.position, 1.0f);
}
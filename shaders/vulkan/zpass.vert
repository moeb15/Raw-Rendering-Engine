#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout(push_constant) uniform constants{
	mat4 transform;
	uint materialIndex;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{	
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	gl_Position = GlobalSceneData.viewProj * PushConstants.transform * vec4(v.position, 1.0f);
}
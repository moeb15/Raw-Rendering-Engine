#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outTangent;
layout (location = 3) out vec4 outViewSpacePos;
layout (location = 4) out uint outMaterialIndex;

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

	//output the position of each vertex
	gl_Position = GlobalSceneData.viewProj * PushConstants.transform * vec4(v.position, 1.0f);
	outUV = vec2(v.u, v.v);
	outNormal = v.normal;
	outMaterialIndex = PushConstants.materialIndex;
	outTangent = v.tangent;
    outViewSpacePos = GlobalSceneData.view * PushConstants.transform * vec4(v.position, 1.0f);
}
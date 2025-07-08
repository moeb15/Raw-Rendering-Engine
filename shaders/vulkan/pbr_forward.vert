#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 1) out vec2 outUV;
layout (location = 2) out uvec4 outTextures;
layout (location = 3) out vec4 outWorldPos;
layout (location = 4) out vec3 outViewPos;
layout (location = 5) out vec3 outNormal;
layout (location = 6) out uint outEmissive;
layout (location = 7) out float outCutoff;
layout (location = 8) out vec2 outRmFactor;
layout (location = 9) out vec4 outBaseColorFactor;
layout (location = 10) out vec4 outLightPos;

layout (set = 0, binding = 0) uniform sceneData{
	mat4 view;	
	mat4 proj;	
	mat4 viewProj;	
	mat4 lightView;
	mat4 lightProj;
	vec4 lightDir;
	float lightIntensity;
	uint shadowMapIndex;
} GlobalSceneData;

struct Vertex{
	vec3 position;
	float u;
	vec3 normal;
	float v;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout(push_constant) uniform constants{
	mat4 transform;
	uvec4 bindlessTextures;
	uint emissiveTexture;
	float alphaCutoff;
	vec2 rmFactor;
	vec4 baseColorFactor;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{	
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output the position of each vertex
	gl_Position = GlobalSceneData.viewProj * PushConstants.transform * vec4(v.position, 1.0f);
	outUV = vec2(v.u, v.v);
	outTextures = PushConstants.bindlessTextures;
	outWorldPos = PushConstants.transform * vec4(v.position, 1.0f);
	outViewPos = outWorldPos.xyz / outWorldPos.w;
	outNormal = v.normal;
	outEmissive = PushConstants.emissiveTexture;
	outCutoff = PushConstants.alphaCutoff;
	outRmFactor = PushConstants.rmFactor;
	outBaseColorFactor = PushConstants.baseColorFactor;
	outLightPos = GlobalSceneData.lightProj * GlobalSceneData.lightView * PushConstants.transform * vec4(v.position, 1.0f);
}
#define MAX_MATERIALS 512
#define INVALID_MATERIAL_INDEX 4294967295

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

struct PBRMaterial
{
    int diffuse;
    int roughness;
    int normal;
    int occlusion;
    vec2 metalRoughnessFactor;
    int emissive;
    float alphaCutoff;
    vec4 baseColorFactor;
    uint isTransparent;
};

struct Vertex{
	vec3 position;
	float u;
	vec3 normal;
	float v;
	vec4 tangent;
};
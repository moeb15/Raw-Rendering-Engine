#define MAX_MATERIALS 512
#define INVALID_MATERIAL_INDEX 4294967295
#define PI 3.1415926538
#define AMBIENT 0.5
#define BIAS 0.0005

layout (set = 0, binding = 0) uniform sceneData{
	mat4 view;
    mat4 viewInv;	
	mat4 proj;
    mat4 projInv;	
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

struct MeshDrawData
{
    mat4 transform;
    uint materialIndex;
    uint isTransparent;
};

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

float distGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

float geomSchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0) * 0.5;
    float k = r * r;
    float denom = NdotV * (1.0 - k) + k;
    
    return NdotV / denom;
}

float geomSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geomSchlickGGX(NdotV, roughness);
    float ggx2 = geomSchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
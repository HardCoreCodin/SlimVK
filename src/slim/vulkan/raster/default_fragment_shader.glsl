#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi (3.14159265f)
#define TAU (2.0f*pi)
#define UNIT_SPHERE_AREA_OVER_SIX ((4.0f*pi)/6.0f)
#define ONE_OVER_PI (1.0f/pi)

#define HAS_ALBEDO_MAP 1
#define HAS_NORMAL_MAP 2
#define DRAW_DEPTH 4
#define DRAW_POSITION 8
#define DRAW_NORMAL 16
#define DRAW_UVS 32
#define DRAW_ALBEDO 64
#define DRAW_IBL 128
#define DRAW_SHADOW_MAP 256


#define EPS 0.0001f

struct Light 
{
	vec4 color_and_intensity;
	vec4 position_or_direction_and_type;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform LightingUniform {
    mat4 directional_light_transform;
    vec4 camera_position_and_IBL_intensity;
    Light lights[4];
} lighting;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;
layout(set = 1, binding = 1) uniform sampler2D normal_texture;
layout(set = 2, binding = 0) uniform samplerCube radiance_map;
layout(set = 2, binding = 1) uniform samplerCube irradiance_map;
layout(set = 3, binding = 0) uniform sampler2D directional_shadow_map_texture;

struct ModelMaterialParams {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;
    float normal_strength;
    uint flags;
};

struct ModelTransform {
    vec4 rotation;
    vec3 position;
    vec3 scale;
};

layout(push_constant) uniform Model {
    ModelTransform transform;
    ModelMaterialParams material_params;
} model;


struct Radiance {
	vec3 Fd;
	vec3 Fs;
};

vec3 rotateNormal(vec3 Ng, vec4 normal_sample, float magnitude) {
    vec3 Nm = normalize(normal_sample.xzy * 2.0f - 1.0f);
    float angle = magnitude * acos(Nm.y) * 0.5f;
    vec4 q = normalize(vec4(normalize(vec3(Nm.z, 0, -Nm.x)) * sin(angle), cos(angle)));
    vec3 T = cross(q.xyz, Ng);
    return Ng + 2.0f * (T * q.w + cross(q.xyz, T));
}

vec3 decodeNormal(const vec4 color) {
    return normalize(color.xyz * 2.0f - 1.0f);
}

float ggxTrowbridgeReitz_D(float roughness, float NdotH) { // NDF
    float a = roughness * roughness;
    float denom = NdotH * NdotH * (a - 1.0f) + 1.0f;
    return (
        a
        /
        (pi * denom * denom)
    );
}

float ggxSchlickSmith_G(float roughness, float NdotL, float NdotV) {
    float k = roughness * roughness * 0.5f;
    return (
        NdotV / max(mix(NdotV, 1.0f, k), EPS) *
        NdotL / max(mix(NdotL, 1.0f, k), EPS)
    );
}

vec3 schlickFresnel(float HdotL, vec3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - HdotL, 5.0f);
}

vec3 cookTorrance(float roughness, float NdotL, float NdotV, float NdotH, vec3 F) {
    float D = ggxTrowbridgeReitz_D(roughness, NdotH);
    float G = ggxSchlickSmith_G(roughness, NdotL, NdotV);
    return (
        F * (D * G
        / (
        4.0f * NdotL * NdotV
    )));
}


Radiance BRDF(vec3 albedo, vec3 V, vec3 N, float NdotL, vec3 L) {
	Radiance rad;
    rad.Fs = vec3(0.0);
    rad.Fd = albedo * ((1.0f - model.material_params.metalness) * ONE_OVER_PI);
	float NdotV = dot(N, V);
    if (NdotV > 0.0f && model.material_params.roughness > 0) {
		vec3 H = normalize(L + V);
		float NdotH = clamp(dot(N, H), 0.0, 1.0);
		float HdotL = clamp(dot(H, L), 0.0, 1.0);
		vec3 F = schlickFresnel(HdotL, model.material_params.F0);
		rad.Fs = cookTorrance(model.material_params.roughness, NdotL, NdotV, NdotH, F);
		rad.Fd *= 1.0f - F;
	}
    
	return rad;
}

vec3 shadeFromLight(vec3 light, vec3 L, vec3 N, vec3 albedo) {    
	float NdotL = dot(L, N);
    if (NdotL <= 0.0f)
        return vec3(0.0f);

    float Ld = length(L);
    NdotL /= Ld;
    vec3 V = normalize(lighting.camera_position_and_IBL_intensity.xyz - in_position);

    Radiance rad = BRDF(albedo, V, N, NdotL, L);
    return (rad.Fd + rad.Fs) * light * NdotL;
}

float calcShadowFactorForDirectionalLight(vec3 lightDir, vec3 N)
{
    vec4 directional_light_space_pos = lighting.directional_light_transform * vec4(in_position, 1.0);
	vec3 projCoords = directional_light_space_pos.xyz / directional_light_space_pos.w;
	float currentDepth = projCoords.z;	
	projCoords = projCoords * 0.5 + 0.5;
	float closestDepth = texture(directional_shadow_map_texture, projCoords.xy).r;
	
	float bias = max(0.05 * (1.0 - dot(N, lightDir)), 0.0005);
	
	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(directional_shadow_map_texture, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(directional_shadow_map_texture, projCoords.xy + vec2(x,y) * texel_size).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	
	shadow /= 9.0;
	if(projCoords.z > 1.0)
	{
		shadow = 0.0;
	}
	
	return shadow;
}

vec3 shadeFromDirectionalLight(vec3 radiance, vec3 L, vec3 N, vec3 albedo)
{
	float shadow = calcShadowFactorForDirectionalLight(L, N);
	return (1.0 - shadow) * shadeFromLight(radiance, L, N, albedo);
}

vec3 shadeFromPointLight(vec3 radiance, vec3 position, vec3 N, vec3 albedo)
{
	vec3 L = position - in_position;
	float distance = length(L);
	L = normalize(L);
	
	float shadow = 0.0f;// CalcPointShadowFactor(pLight, shadowIndex);
	return (1.0 - shadow) * shadeFromLight(radiance, L, N, albedo) / (distance * distance);
}

vec3 toneMapped(vec3 color) {
    vec3 x = clamp(color - 0.004f, 0.0f, 1.0f);
    vec3 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}

void main() {
    vec3 albedo = model.material_params.albedo;
    if ((model.material_params.flags & HAS_ALBEDO_MAP) != 0) {
        albedo *= texture(albedo_texture, in_uv).rgb;
    }
    const vec3 C = lighting.camera_position_and_IBL_intensity.xyz;
    const vec3 P = in_position;
    vec3 N = normalize(in_normal);
    if ((model.material_params.flags & HAS_NORMAL_MAP) != 0) {
        N = normalize(rotateNormal(N, texture(normal_texture, in_uv), model.material_params.normal_strength));
    }

    vec3 color = vec3(0.0f);
    for (int i = 0; i < 4; i++)
    {
        Light light = lighting.lights[i];
        vec3 radiance = light.color_and_intensity.xyz * light.color_and_intensity.w;  
        if (light.position_or_direction_and_type.w == 0.0f)
        {
            vec3 direction = light.position_or_direction_and_type.xyz;
            color += shadeFromDirectionalLight(radiance, direction, N, albedo);
        }
        else
        {
            vec3 position = light.position_or_direction_and_type.xyz;
            color += shadeFromPointLight(radiance, position, N, albedo);
        }
    }

    const float IBL_intensity = lighting.camera_position_and_IBL_intensity.w;
    if (IBL_intensity != 0.0f) {
        vec3 V = normalize(C - P);
		Radiance rad = BRDF(albedo, V, N, 1.0f, N);
		rad.Fd *= texture(irradiance_map, N).rgb;
		rad.Fs *= texture(radiance_map, normalize(reflect(V, N))).rgb;
		color += (rad.Fd + rad.Fs) * IBL_intensity;
	} 

    out_color = vec4(toneMapped(color), 1.0f);
}
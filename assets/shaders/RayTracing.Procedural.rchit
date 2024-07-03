#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require

#include "common/Material.glsl"
#include "common/UniformBufferObject.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT Scene;
layout(binding = 1) readonly buffer LightObjectArray { LightObject[] Lights; };
layout(binding = 3) readonly uniform UniformBufferObjectStruct { UniformBufferObject Camera; };
layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 6) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 7) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 8) uniform sampler2D[] TextureSamplers;
layout(binding = 9) readonly buffer SphereArray { vec4[] Spheres; };

#include "common/Const_Func.glsl"
#include "common/Scatter.glsl"
#include "common/Vertex.glsl"

hitAttributeEXT vec4 Sphere;
rayPayloadInEXT RayPayload Ray;

vec2 GetSphereTexCoord(const vec3 point)
{
	const float theta = asin(point.y);

	return vec2
	(
		(atan(point.x, point.z) + M_PI),
		2 - (theta + theta + M_PI)
	) * M_1_OVER_TWO_PI;
}

void main()
{
	// Get the material.
	const uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
	const uint indexOffset = offsets.x;
	const uint vertexOffset = offsets.y;
	const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset]);
	const Material material = Materials[v0.MaterialIndex];

	// Compute the ray hit point properties.
	const vec4 sphere = Spheres[gl_InstanceCustomIndexEXT];
	const vec3 center = sphere.xyz;
	const float radius = sphere.w;
	const vec3 point = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
	const vec3 normal = normalize( (point - center) / radius );
	const vec2 texCoord = GetSphereTexCoord(normal);

    int lightIdx = int(floor(RandomFloat(Ray.RandomSeed) * .99999 * Lights.length()));

    Ray.primitiveId = v0.MaterialIndex + 1;
	Ray.BounceCount++;
	Ray.Exit = false;
	Scatter(Ray, material, Lights[lightIdx], gl_WorldRayDirectionEXT, normal, texCoord, gl_HitTEXT, v0.MaterialIndex);
}

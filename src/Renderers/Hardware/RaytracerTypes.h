#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/hash.hpp>

struct RaytracePushConstants
{
	glm::vec3 pixel00Location;
    int raysPerPixel = 1;

	glm::vec3 pixelDeltaU;
    int maxBounces = 3;

	glm::vec3 pixelDeltaV;
    float defocusAngle;

	glm::vec3 cameraPosition;
    int parentAABBCount;

	glm::vec3 defocusDiskU;
	int accumulateFrames;

	glm::vec3 defocusDiskV;
    int frame = 0;

	glm::vec3 sunDirection = glm::vec3(-0.5, -1.0, -0.3);
	float sunIntensity = 5.0f;

	glm::vec3 sunColour = glm::vec3(1.0, 0.95, 0.85);
	float padding;
};

struct GPUAABB
{
	glm::vec3 min;
	int objectIndex;
	glm::vec3 max;
	float padding;
};

struct GPUSphere
{
	glm::vec3 center;
	float radius;
	int materialIndex;
	float padding;
	float padding2;
	float padding3;
};

struct GPUTriangle
{
	glm::vec3 v0;
	float padding;

	glm::vec3 v1;
	float padding1;

	glm::vec3 v2;
	float padding2;

	glm::vec2 uv0;
	glm::vec2 uv1;

	glm::vec2 uv2;
	float padding4;
	float padding5;

	glm::vec3 n0;
	float padding6;

	glm::vec3 n1;
	float padding7;

	glm::vec3 n2;
	float padding8;
};

struct GPUObject
{
	int triangleStartIndex;
	int triangleCount;
	int materialIndex;
	float padding;

	glm::mat4 inverseTransform;
};

struct GPUMaterial
{
	glm::vec3 albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	float smoothness = 0.0f;
	float fuzziness = 0.0f;

	float emission = 0.0f;
	float refractiveIndex = 0.0f;
	float padding1;

	glm::vec3 absorbtion = glm::vec3(0,0,0);
	float padding3;

	bool operator==(const GPUMaterial& other) const
	{
		return (albedo == other.albedo) &&
			(smoothness == other.smoothness) &&
			(fuzziness == other.fuzziness) &&
			(emission == other.emission) &&
			(refractiveIndex == other.refractiveIndex) &&
			(absorbtion == other.absorbtion);
	}
};
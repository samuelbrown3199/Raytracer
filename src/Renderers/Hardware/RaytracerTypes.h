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
    int raysPerPixel = 10;

	glm::vec3 pixelDeltaU;
    int maxBounces = 3;

	glm::vec3 pixelDeltaV;
    float defocusAngle;

	glm::vec3 cameraPosition;
    int sphereCount;

	glm::vec3 defocusDiskU;
	int padding2;

	glm::vec3 defocusDiskV;
    int frame = 0;

	glm::vec3 sunDirection = glm::vec3(-0.5, -1.0, -0.3);
	float sunIntensity = 5.0f;

	glm::vec3 sunColour = glm::vec3(1.0, 0.95, 0.85);
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

struct GPUMaterial
{
	glm::vec3 albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	float smoothness = 0.0f;

	float emission = 0.0f;
	float refractiveIndex = 0.0f;
	float padding1;
	float padding2;

	glm::vec3 absorbtion = glm::vec3(0,0,0);
	float padding3;
};
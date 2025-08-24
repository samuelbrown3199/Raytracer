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
	int renderMode;

	glm::vec3 defocusDiskV;
    int frame;
};

struct GPUSphere
{
	glm::vec3 center;
	float radius;
	glm::vec3 colour;
	float padding;
};
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

struct AllocatedImage
{
    VkImage m_image;
    VkImageView m_imageView;
    VmaAllocation m_allocation = nullptr;
    VkExtent3D m_imageExtent;
    VkFormat m_imageFormat;
};

struct AllocatedBuffer
{
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_info;
};

struct Vertex
{
    glm::vec3 m_position = glm::vec3(0, 0, 0);
    float m_uvX = 0;
    glm::vec3 m_normal = glm::vec3(0, 0, 0);
    float m_uvY = 0;

    bool operator==(const Vertex& other) const
    {
        return m_position == other.m_position && m_normal == other.m_normal && m_uvX == other.m_uvX && m_uvY == other.m_uvY;
    }

    Vertex() {};
    Vertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal)
    {
        m_position = pos;
        m_normal = normal;

        m_uvX = uv.x;
        m_uvY = uv.y;
    }
};

struct ChunkVertex
{
    glm::vec3 m_position;   // 12B
    float m_pad0 = 0.0f;    // Pad to 16B

    glm::vec3 m_normal;     // 12B
    float m_sunlightLevel = 0.0f;

    glm::vec2 m_uv;         // 8B
    int m_texLayer = 0;     // 4B
    int m_pad2 = 0;         // Pad to 16B total

    bool operator==(const ChunkVertex& other) const
    {
        return m_position == other.m_position &&
            m_normal == other.m_normal &&
            m_uv == other.m_uv &&
            m_texLayer == other.m_texLayer;
    }

    ChunkVertex() = default;
    ChunkVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal, int texLayer, float sunlightLevel)
        : m_position(pos), m_normal(normal), m_uv(uv), m_texLayer(texLayer), m_sunlightLevel(sunlightLevel) {
    }
};

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.m_position) ^
                (hash<glm::vec3>()(vertex.m_normal) << 1) >> 1 ^
                (hash<float>()(vertex.m_uvX) << 1) >> 1 ^
                (hash<float>()(vertex.m_uvY) << 1) >> 1));
        }
    };
}

struct GPUMeshBuffers
{
    AllocatedBuffer m_indexBuffer;
    AllocatedBuffer m_vertexBuffer;
    VkDeviceAddress m_vertexBufferAddress;
};

struct GPUPointLight
{
    alignas(16) glm::vec3 position;

    alignas(16) glm::vec3 diffuseLight;
    alignas(16) glm::vec3 specularLight;

    alignas(4) float constant;
    alignas(4) float linear;
    alignas(4) float quadratic;
    alignas(4) float intensity;
};

struct GPUDirectionalLight
{
    alignas(16) glm::vec3 direction;
    alignas(4) float intensity;

    alignas(16) glm::vec3 diffuseLight;
    alignas(16) glm::vec3 specularLight;
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;

    alignas(16) glm::vec3 viewPos;
	alignas(16) glm::vec3 directionalLightDirection;
};

struct GPUDrawPushConstants
{
    glm::mat4 m_worldMatrix;
    glm::vec4 m_objectColour;
    VkDeviceAddress m_vertexBuffer;
};

struct GPUChunkPushConstants
{
    glm::mat4 m_worldMatrix;
    VkDeviceAddress m_vertexBuffer;

	float m_daylightLevel = 1.0f;
    float m_time;
};
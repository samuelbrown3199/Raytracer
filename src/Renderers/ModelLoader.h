#pragma once

#include <vector>
#include <string>

#include "Hardware/Vulkan/VulkanTypes.h"
#include "Hardware/RaytracerTypes.h"

void BuildBVH(std::vector<GPUTriangle>& triangles, std::vector<GPUBVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize);
void SplitBVHNode(std::vector<GPUTriangle>& triangles, std::vector<GPUBVHNode>& outNodes, int& currentNodeIndex);
void ExpandNodeAABB(std::vector<GPUTriangle>& triangles, GPUBVHNode& child);
void LoadObjFile(const std::string& filePath, std::vector<Vertex>& vertices);
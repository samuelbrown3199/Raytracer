#pragma once

#include <vector>
#include <string>

#include "Hardware/Vulkan/VulkanTypes.h"
#include "Hardware/RaytracerTypes.h"

void BuildBVH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize);
void SplitBVHNode(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, int& currentNodeIndex);
void ExpandNodeAABB(std::vector<Triangle>& triangles, BVHNode& child);
void LoadObjFile(const std::string& filePath, std::vector<Vertex>& vertices);
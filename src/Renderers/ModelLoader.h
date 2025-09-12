#pragma once

#include <vector>
#include <string>

#include "Hardware/Vulkan/VulkanTypes.h"
#include "Hardware/RaytracerTypes.h"

void BuildBVH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize);
void SplitBVHNode(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, int& currentNodeIndex);

void BuildBVHSAH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize);
float EvaluateSplitCost(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, BVHNode& node, int axis, float pos);
void SplitBVHNodeSAH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, BVHNode& node);

void ExpandNodeAABB(std::vector<Triangle>& triangles, BVHNode& child);
void LoadObjFile(const std::string& filePath, std::vector<Vertex>& vertices);
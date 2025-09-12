#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

#include "../Useful/Useful.h"

void BuildBVH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize)
{
	//No need to split further
	if (triangles.size() <= 2)
		return;

	//Determine longest axis
	int axis = parentNode.node.aabb.GetLongestAxis();
	float splitPos = (parentNode.node.aabb.min[axis] + parentNode.node.aabb.max[axis]) * 0.5f;
	int i = 0;
	int j = triangles.size() - 1;
	while (i <= j)
	{
		if (triangles[i].triCentroid[axis] < splitPos)
		{
			i++;
		}
		else
		{
			std::swap(triangles[i], triangles[j]);
			j--;
		}
	}

	int leftCount = i;
	if (leftCount == 0 || leftCount == triangles.size())
		return;

	BVHNode leftChild;
	leftChild.triangleStartIndex = 0;
	leftChild.triangleCount = leftCount;
	leftChild.aabb = AABB();

	BVHNode rightChild;
	rightChild.triangleStartIndex = leftCount;
	rightChild.triangleCount = triangles.size() - leftCount;
	rightChild.aabb = AABB();

	ExpandNodeAABB(triangles, leftChild);
	ExpandNodeAABB(triangles, rightChild);

	outNodes.push_back(leftChild);
	outNodes.push_back(rightChild);

	int leftChildIndex = 0;
	int rightChildIndex = 1;

	SplitBVHNode(triangles, outNodes, leftChildIndex);
	SplitBVHNode(triangles, outNodes, rightChildIndex);

	parentNode.node.leftChild = currentBVHSize;
	parentNode.node.rightChild = currentBVHSize + 1;
}

void SplitBVHNode(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, int& currentNodeIndex)
{
	BVHNode& node = outNodes[currentNodeIndex];

	node.leftChild = -1;
	node.rightChild = -1;

	//No need to split further
	if (node.triangleCount <= 2)
		return;

	//Determine longest axis
	int axis = node.aabb.GetLongestAxis();
	float splitPos = (node.aabb.min[axis] + node.aabb.max[axis]) * 0.5f;
	int i = node.triangleStartIndex;
	int j = node.triangleStartIndex + node.triangleCount - 1;
	while (i <= j)
	{
		if (triangles[i].triCentroid[axis] < splitPos)
		{
			i++;
		}
		else
		{
			std::swap(triangles[i], triangles[j]);
			j--;
		}
	}
	int leftCount = i - node.triangleStartIndex;
	if (leftCount == 0 || leftCount == node.triangleCount)
		return;

	int leftChildIndex = outNodes.size();
	int rightChildIndex = outNodes.size() + 1;

	node.leftChild = leftChildIndex;
	node.rightChild = rightChildIndex;

	BVHNode leftChild;
	leftChild.triangleStartIndex = node.triangleStartIndex;
	leftChild.triangleCount = leftCount;
	leftChild.aabb = AABB();

	BVHNode rightChild;
	rightChild.triangleStartIndex = i;
	rightChild.triangleCount = node.triangleCount - leftCount;
	rightChild.aabb = AABB();

	ExpandNodeAABB(triangles, leftChild);
	ExpandNodeAABB(triangles, rightChild);

	outNodes.push_back(leftChild);
	outNodes.push_back(rightChild);

	SplitBVHNode(triangles, outNodes, leftChildIndex);
	SplitBVHNode(triangles, outNodes, rightChildIndex);
}

//WIP - SAH version

void BuildBVHSAH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, ParentBVHNode& parentNode, int currentBVHSize)
{
	//No need to split further
	if (triangles.size() <= 2)
		return;

	BVHNode leftChild;
	leftChild.leftChild = -1;
	leftChild.aabb = AABB();

	BVHNode rightChild;
	rightChild.leftChild = -1;
	rightChild.aabb = AABB();

	outNodes.push_back(leftChild);
	outNodes.push_back(rightChild);

	SplitBVHNodeSAH(triangles, outNodes, leftChild);
	SplitBVHNodeSAH(triangles, outNodes, rightChild);

	parentNode.node.leftChild = currentBVHSize;
	parentNode.node.rightChild = currentBVHSize + 1;
}

void SplitBVHNodeSAH(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, BVHNode& node)
{
	node.leftChild = -1;
	node.rightChild = -1;

	//No need to split further
	if (node.triangleCount <= 2)
		return;

	int leftChildIndex = outNodes.size();
	int rightChildIndex = outNodes.size() + 1;

	node.leftChild = leftChildIndex;
	node.rightChild = rightChildIndex;

	BVHNode leftChild;
	leftChild.leftChild = -1;
	leftChild.aabb = AABB();

	BVHNode rightChild;
	rightChild.leftChild = -1;
	rightChild.aabb = AABB();

	outNodes.push_back(leftChild);
	outNodes.push_back(rightChild);

	int bestAxis = -1;
	float bestPos = 0.0f, bestCost = 1e30f;
	for (int a = 0; a < 3; a++)
	{
		for (int i = 0; i < node.triangleCount; i++)
		{
			Triangle& tri = triangles[node.triangleStartIndex + i];
			float centroidPos = tri.triCentroid[a];
			if (centroidPos > node.aabb.min[a] && centroidPos < node.aabb.max[a])
			{
				float cost = EvaluateSplitCost(triangles, outNodes, node, a, centroidPos);
				if (cost < bestCost)
				{
					bestCost = cost;
					bestAxis = a;
					bestPos = centroidPos;
				}
			}
		}
	}

	float splitPos = bestPos;
	int i = node.triangleStartIndex;
	int j = node.triangleStartIndex + node.triangleCount - 1;

	glm::vec3 e = node.aabb.max - node.aabb.min;
	float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
	float parentCost = node.triangleCount * parentArea;

	while (i <= j)
	{
		if (triangles[i].triCentroid[bestAxis] < splitPos)
		{
			i++;
		}
		else
		{
			std::swap(triangles[i], triangles[j]);
			j--;
		}
	}
	int leftCount = i - node.triangleStartIndex;
	if (leftCount == 0 || leftCount == node.triangleCount)
		return;

	if (bestCost >= parentCost) return;

	SplitBVHNodeSAH(triangles, outNodes, leftChild);
	SplitBVHNodeSAH(triangles, outNodes, rightChild);
}

float EvaluateSplitCost(std::vector<Triangle>& triangles, std::vector<BVHNode>& outNodes, BVHNode& node, int axis, float pos)
{
	BVHNode& leftChild = outNodes[node.leftChild];
	BVHNode& rightChild = outNodes[node.rightChild];

	AABB& leftAABB = leftChild.aabb;
	AABB& rightAABB = rightChild.aabb;
	int leftCount = 0, rightCount = 0;
	for (int i = 3; i < node.triangleCount; i++)
	{
		Triangle& tri = triangles[node.triangleStartIndex + i];
		if (tri.triCentroid[axis] < pos)
		{
			leftCount++;
			leftAABB.Grow(tri.v0);
			leftAABB.Grow(tri.v1);
			leftAABB.Grow(tri.v2);
		}
		else
		{
			rightCount++;
			rightAABB.Grow(tri.v0);
			rightAABB.Grow(tri.v1);
			rightAABB.Grow(tri.v2);
		}
	}

	leftChild.triangleStartIndex = node.triangleStartIndex;
	leftChild.triangleCount = leftCount;

	rightChild.triangleStartIndex = node.triangleStartIndex + leftCount;
	rightChild.triangleCount = rightCount;

	float cost = leftCount * leftAABB.GetArea() + rightCount * rightAABB.GetArea();
	return cost > 0 ? cost : 1e30f;
}

void ExpandNodeAABB(std::vector<Triangle>& triangles, BVHNode& child)
{
	child.aabb.min = glm::vec3(FLT_MAX);
	child.aabb.max = glm::vec3(-FLT_MAX);

	int first = child.triangleStartIndex;
	for (int i = 0; i < child.triangleCount; i++)
	{
		Triangle& tri = triangles[first + i];
		if (tri.v0.x < child.aabb.min.x) child.aabb.min.x = tri.v0.x;
		if (tri.v0.y < child.aabb.min.y) child.aabb.min.y = tri.v0.y;
		if (tri.v0.z < child.aabb.min.z) child.aabb.min.z = tri.v0.z;
		if (tri.v0.x > child.aabb.max.x) child.aabb.max.x = tri.v0.x;
		if (tri.v0.y > child.aabb.max.y) child.aabb.max.y = tri.v0.y;
		if (tri.v0.z > child.aabb.max.z) child.aabb.max.z = tri.v0.z;
		if (tri.v1.x < child.aabb.min.x) child.aabb.min.x = tri.v1.x;
		if (tri.v1.y < child.aabb.min.y) child.aabb.min.y = tri.v1.y;
		if (tri.v1.z < child.aabb.min.z) child.aabb.min.z = tri.v1.z;
		if (tri.v1.x > child.aabb.max.x) child.aabb.max.x = tri.v1.x;
		if (tri.v1.y > child.aabb.max.y) child.aabb.max.y = tri.v1.y;
		if (tri.v1.z > child.aabb.max.z) child.aabb.max.z = tri.v1.z;
		if (tri.v2.x < child.aabb.min.x) child.aabb.min.x = tri.v2.x;
		if (tri.v2.y < child.aabb.min.y) child.aabb.min.y = tri.v2.y;
		if (tri.v2.z < child.aabb.min.z) child.aabb.min.z = tri.v2.z;
		if (tri.v2.x > child.aabb.max.x) child.aabb.max.x = tri.v2.x;
		if (tri.v2.y > child.aabb.max.y) child.aabb.max.y = tri.v2.y;
		if (tri.v2.z > child.aabb.max.z) child.aabb.max.z = tri.v2.z;
	}
}

void LoadObjFile(const std::string& filePath, std::vector<Vertex>& vertices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str()))
    {
        throw std::exception(FormatString("Failed to load OBJ file %s", filePath).c_str());
    }

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.m_position[0] = attrib.vertices[3 * index.vertex_index + 0];
            vertex.m_position[1] = attrib.vertices[3 * index.vertex_index + 1];
            vertex.m_position[2] = attrib.vertices[3 * index.vertex_index + 2];

            /*if (attrib.texcoords.size() != 0)
            {
                vertex.m_uvX = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.m_uvY = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
            }*/

            if (attrib.normals.size() != 0)
            {
                vertex.m_normal[0] = attrib.normals[3 * index.normal_index + 0];
                vertex.m_normal[1] = attrib.normals[3 * index.normal_index + 1];
                vertex.m_normal[2] = attrib.normals[3 * index.normal_index + 2];
            }

            vertices.push_back(vertex);
        }
    }
}
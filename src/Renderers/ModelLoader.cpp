#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

#include "../Useful/Useful.h"

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
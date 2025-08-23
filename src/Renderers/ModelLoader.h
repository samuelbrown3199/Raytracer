#pragma once

#include <vector>
#include <string>

struct Vertex
{
	double vertexPos[3];
	double normals[3];
	double texCoords[2];
};

void LoadObjFile(const std::string& filePath, std::vector<Vertex>& vertices);
#pragma once
#include <vector>
#include <glm/matrix.hpp>
#include <string>
#include <filesystem>
#include "Mesh.h"


class RenderObject
{
public:
	RenderObject();
	~RenderObject();

	std::vector<RenderObject> children;

	glm::mat4 worldMatrix;
	std::string meshId;
	std::string materialId;

	bool load_from_gltf(std::filesystem::path gltf_path);
private:

};



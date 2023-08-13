#include "RenderObject.h"
#include <chrono>
#include <tiny_gltf.h>
#include <iostream>


RenderObject::RenderObject()
{
	worldMatrix = glm::mat4(1.0);
}

RenderObject::~RenderObject()
{
}


bool RenderObject::load_from_gltf(std::filesystem::path gltf_path)
{

	tinygltf::Model model;
	std::string err;
	std::string warn;
	tinygltf::TinyGLTF loader;

	const std::chrono::time_point<std::chrono::steady_clock>  load_start = std::chrono::steady_clock::now();

	bool load_status = loader.LoadBinaryFromFile(&model, &err, &warn, gltf_path.generic_string());
	auto load_time = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - load_start);
	std::cout << "GLTF loading time: " << load_time << " from file: " << gltf_path << std::endl;

	if (!warn.empty()) {
		std::cout << "GLTF warning: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cout << "GLTF error: " << err << std::endl;
	}

	return load_status;
}
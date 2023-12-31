#pragma once
#include <vk_types.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <filesystem>

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
	VkPipelineVertexInputStateCreateFlags flags;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color; //lamenting for colour

	static VertexInputDescription get_vertex_description();
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Mesh {
	glm::mat4 mvpMatrix;
	std::vector<Vertex> vertices;
	AllocatedBuffer vertexBuffer;

	//Plan A:
	bool load_from_gltf(std::filesystem::path);
	//In case of big stoopid:
	bool load_from_obj(std::filesystem::path);
};
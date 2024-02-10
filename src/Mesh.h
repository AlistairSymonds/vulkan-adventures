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
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
	static VertexInputDescription get_vertex_description();
};

struct MeshPushConstants {
	glm::mat4 render_matrix;
	uint64_t vb;
};

struct GPUMeshBuffers {

	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};


struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	GPUMeshBuffers buffer;

	//In case of big stoopid:
	bool load_from_obj(std::filesystem::path);
};
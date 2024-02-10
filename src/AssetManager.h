#pragma once
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "DeletionQueue.h"
#include "RenderEngine.h"
#include "RenderObject.h"
#include "Mesh.h"

class AssetManager
{
public:
	AssetManager(VkInstance inst, VkDevice dev, VkQueue xfer_q, uint32_t xfer_q_idx, VmaAllocator alloc);
	~AssetManager();

	void init_vk();
	
	VkShaderModule getShaderModule(std::string id);
	Mesh getMesh(std::string);
	void loadAssets();
	void cleanup();

private:

	VmaAllocator allocator; //already an opaque pointer
	VkDevice device;
	VkQueue transfer_q;
	uint32_t transfer_q_index;
	VkCommandPool cmdPool;

	bool load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule);
	void load_all_shader_modules();
	std::unordered_map<std::string, VkShaderModule> shader_modules;

	void upload_mesh(Mesh& mesh);
	std::unordered_map<std::string, Mesh> meshes;

	void load_render_objects();
	std::unordered_map<std::string, RenderObject> knownRenderObjs;

	void load_meshes();

	AllocatedBuffer staging;
	void init_staging_buffer();
	void add_to_staging_buffer(AllocatedBuffer gpu_dst, void* src, size_t size);
	VkResult flush_staging_buffer_to_gpu();
	void initDefaultMesh();
	void initDefaultTexture();
	DeletionQueue cleanup_queue;

	VkInstance instance;
	PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
};

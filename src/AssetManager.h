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
	AssetManager(VkInstance inst, VkDevice dev, VmaAllocator alloc);
	~AssetManager();
	
	VkShaderModule getShaderModule(std::string id);
	Mesh getMesh(std::string);
	void loadAssets();
	void cleanup();

private:

	VmaAllocator allocator; //already an opaque pointer
	VkDevice device;

	bool load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule);
	void load_all_shader_modules();
	std::unordered_map<std::string, VkShaderModule> shader_modules;

	void upload_mesh(Mesh& mesh);
	std::unordered_map<std::string, Mesh> meshes;

	void load_render_objects();
	std::unordered_map<std::string, RenderObject> knownRenderObjs;

	void load_meshes();
	void initDefaultMesh();
	void initDefaultTexture();
	DeletionQueue cleanup_queue;

	VkInstance instance;
	PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
};

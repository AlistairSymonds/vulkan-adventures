#pragma once
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "Mesh.h"
class AssetManager
{
public:
	AssetManager(VkDevice dev);
	~AssetManager();
	
	VkShaderModule getShaderModule(std::string id);
	Mesh getMesh(std::string);
	void loadAssets();
	void cleanup();

private:

	VkDevice device;
	bool load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule);
	void load_all_shader_modules();
	std::unordered_map<std::string, VkShaderModule> shader_modules;

};

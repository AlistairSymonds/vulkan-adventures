#include <fstream>
#include <iostream>
#include "AssetManager.h"

AssetManager::AssetManager(VkDevice dev)
{
    device = dev;
}

AssetManager::~AssetManager()
{
}

void AssetManager::loadAssets()
{
    load_all_shader_modules();
}

void AssetManager::cleanup() {
    for (auto sm : shader_modules) {
        vkDestroyShaderModule(device, sm.second, nullptr);
    }
}


bool AssetManager::load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule)
{
    //open the file. With cursor at the end
    std::ifstream file(shader_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }
    //find what the size of the file is by looking up the location of the cursor
    //because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    //spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    //put file cursor at beginning
    file.seekg(0);

    //load the entire file into the buffer
    file.read((char*)buffer.data(), fileSize);

    //now that the file is loaded into the buffer, we can close it
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;


    return true;
}

void AssetManager::load_all_shader_modules() {
    const std::filesystem::path shader_dir("shaders");

    std::cout << "Loading .spv from:" << std::filesystem::absolute(shader_dir) << std::endl;
    for (std::filesystem::path fp : std::filesystem::directory_iterator(shader_dir)) {
        if (fp.extension() == ".spv") {

            std::cout << fp << std::endl;
            VkShaderModule mod;
            load_shader_module(fp, &mod);
            std::string shader_name;
            shader_name = fp.filename().replace_extension().generic_string();
            shader_modules[shader_name] = mod;
        }
    }
    if (shader_modules.size() == 0)
    {
        assert("Didn't load any shaders");
    }
}

VkShaderModule AssetManager::getShaderModule(std::string id)
{
    if (!shader_modules.contains(id))
    {
        std::cout << "Couldn't find shader module: " << id << std::endl;
    }
    return shader_modules[id];
}

Mesh AssetManager::getMesh(std::string)
{
	return Mesh();
}
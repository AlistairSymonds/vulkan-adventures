#include <fstream>
#include <iostream>
#include "AssetManager.h"
#include "vk_initializers.h"

AssetManager::AssetManager(VkDevice dev, VmaAllocator alloc)
{
    device = dev;
    allocator = alloc;
}

AssetManager::~AssetManager()
{
}

void AssetManager::loadAssets()
{
    load_all_shader_modules();
    load_meshes();
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

void AssetManager::upload_mesh(Mesh& m) {
    VkBufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = m.vertices.size() * sizeof(Vertex);
    ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vmalloc_ci = {};
    vmalloc_ci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VK_CHECK(vmaCreateBuffer(allocator, &ci, &vmalloc_ci, &m.vertexBuffer.buf,
        &m.vertexBuffer.allocation,
        nullptr));

    cleanup_queue.push_function([=]() {
        vmaDestroyBuffer(allocator, m.vertexBuffer.buf, m.vertexBuffer.allocation);
        });

    void* data;
    vmaMapMemory(allocator, m.vertexBuffer.allocation, &data);

    memcpy(data, m.vertices.data(), m.vertices.size() * sizeof(Vertex));

    vmaUnmapMemory(allocator, m.vertexBuffer.allocation);
}

VkShaderModule AssetManager::getShaderModule(std::string id)
{
    if (!shader_modules.contains(id))
    {
        std::cout << "Couldn't find shader module: " << id << std::endl;
    }
    return shader_modules[id];
}

Mesh AssetManager::getMesh(std::string id)
{
    if (!shader_modules.contains(id))
    {
        std::cout << "Couldn't find Mesh: " << id << std::endl;
    }
    return meshes[id];
}

void AssetManager::load_meshes()
{
    Mesh tri;
    tri.vertices.resize(3);

    //vertex positions
    tri.vertices[0].position = { 1.f, 1.f, 1.0f };
    tri.vertices[1].position = { -1.f, 1.f, 0.0f };
    tri.vertices[2].position = { 0.f,-1.f, 0.0f };

    //vertex colors, all green
    tri.vertices[0].color = { 1.f, 1.f, 0.0f };
    tri.vertices[1].color = { 0.f, 1.f, 0.0f };
    tri.vertices[2].color = { 0.f, 1.f, 1.0f };

    meshes["tri"] = tri;

    Mesh field;
    field.load_from_gltf("C:/Users/alist/source/repos/vulkan-adventures/assets/models/Grass field.glb");
    //meshes["field"] = field;

    for (auto& m : meshes)
    {
        upload_mesh(m.second);
    }
}
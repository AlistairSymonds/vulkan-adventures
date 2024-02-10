#include <fstream>
#include <iostream>
#include <random>
#include <vk_mem_alloc.h>
#include "vk_types.h"
#include "AssetManager.h"
#include "vk_initializers.h"
#include "vk_funcs.h"
#include <tiny_gltf.h>

AssetManager::AssetManager(VkInstance inst, VkDevice dev, VkQueue xfer_q, uint32_t xfer_q_idx, VmaAllocator alloc)
{   
    instance = inst;
    device = dev;
    transfer_q = xfer_q;
    transfer_q_index = xfer_q_idx;
    allocator = alloc;
    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    
    init_vk();
}

AssetManager::~AssetManager()
{
}

void AssetManager::init_vk() {
    //create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(transfer_q_index);
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &cmdPool));

    cleanup_queue.push_function([=]() {
        vkDestroyCommandPool(device, cmdPool, nullptr);
        });

    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &cmdPool));

}

void AssetManager::loadAssets()
{
    load_all_shader_modules();
    init_staging_buffer();
    initDefaultMesh();
    initDefaultTexture();
    load_meshes();
    load_render_objects();
    VK_CHECK(flush_staging_buffer_to_gpu());
}

void AssetManager::cleanup() {
    for (auto sm : shader_modules) {
        vkDestroyShaderModule(device, sm.second, nullptr);
    }
    cleanup_queue.flush();
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

//For a given mesh, create vertex and index buffers on the GPU.
//Then create temporary staging buffer to copy across
void AssetManager::upload_mesh(Mesh& m) {
    const size_t vtxBufSize = m.vertices.size() * sizeof(Vertex);
    const size_t idxBufSize = m.indices.size() * sizeof(uint32_t);
       
    m.buffer.vertexBuffer = vkfuncs::create_buffer(allocator, vtxBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    const VkDebugUtilsObjectNameInfoEXT vtxNameInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = NULL,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)m.buffer.vertexBuffer.buf,
        .pObjectName = "Mesh vtx Buffer",
    };
    pfnSetDebugUtilsObjectNameEXT(device, &vtxNameInfo);

    cleanup_queue.push_function([=]() {
        vmaDestroyBuffer(allocator, m.buffer.vertexBuffer.buf, m.buffer.vertexBuffer.allocation);
        });

    m.buffer.indexBuffer = vkfuncs::create_buffer(allocator, vtxBufSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    const VkDebugUtilsObjectNameInfoEXT idxNameInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = NULL,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)m.buffer.indexBuffer.buf,
        .pObjectName = "Mesh idx Buffer",
    };
    pfnSetDebugUtilsObjectNameEXT(device, &idxNameInfo);

    cleanup_queue.push_function([=]() {
        vmaDestroyBuffer(allocator, m.buffer.indexBuffer.buf, m.buffer.indexBuffer.allocation);
        });

    add_to_staging_buffer(m.buffer.vertexBuffer, &m.vertices, m.vertices.size());
    add_to_staging_buffer(m.buffer.indexBuffer, &m.indices, m.indices.size());
}

void AssetManager::init_staging_buffer() {

    staging = vkfuncs::create_buffer(allocator, 262144, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    cleanup_queue.push_function([=]() {
        vmaDestroyBuffer(allocator, staging.buf, staging.allocation);
        });

}

//copy from CPU location into cpu visible vk staging buffer, then do a copy into the given vk gpu_dst
void AssetManager::add_to_staging_buffer(AllocatedBuffer gpu_dst, void* src, size_t size)
{   
    //Sigh, use this when vma 3.1.1 is released:
    //vmaCopyMemoryToAllocation();
    void* staging_host_ptr;
    vmaMapMemory(allocator, staging.allocation, &staging_host_ptr);
    memcpy(staging_host_ptr, src, size);

    VkFence _immFence;
    VkFenceCreateInfo fi;
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.pNext = NULL;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(device, &fi, nullptr, &_immFence));

    VkCommandBuffer _immCommandBuffer;
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(cmdPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &_immCommandBuffer));

    VK_CHECK(vkResetFences(device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    auto cmd = _immCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    VkBufferCopy copy = {};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(_immCommandBuffer, staging.buf, gpu_dst.buf, 1, &copy);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(transfer_q, 1, &submit, _immFence));
    VK_CHECK(vkWaitForFences(device, 1, &_immFence, true, 9999999999));
    vmaUnmapMemory(allocator, staging.allocation);
}

void AssetManager::load_render_objects()
{
    std::vector<std::filesystem::path> glbs;
    glbs.push_back("C:/Users/alist/source/repos/vulkan-adventures/assets/models/Grass field.glb");
    for (auto g : glbs)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, g.string());

        if (!warn.empty()) {
            printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            printf("Err: %s\n", err.c_str());
        }

        if (!ret) {
            printf("Failed to parse glTF\n");
        }
        RenderObject o;
        for (auto gm : model.meshes) {
            //gm.primitives[0].
        }

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

Mesh AssetManager::getMesh(std::string id)
{
    if (!meshes.contains(id))
    {
        //std::cout << "Couldn't find Mesh: " << id << std::endl;
        return meshes["default_cube"];
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
    tri.vertices[0].color = { 1.f, 1.f, 0.0f, 1.0f };
    tri.vertices[1].color = { 0.f, 1.f, 0.0f, 1.0f };
    tri.vertices[2].color = { 0.f, 1.f, 1.0f, 1.0f };

    tri.indices.resize(3);
    for (size_t i = 0; i < 3; i++)
    {
        tri.indices[i] = i;
    }
    meshes["tri"] = tri;


    for (auto& m : meshes)
    {
        upload_mesh(m.second);
    }
}

VkResult AssetManager::flush_staging_buffer_to_gpu()
{

    return VK_SUCCESS;
}

void AssetManager::initDefaultMesh() {
    //It's a cube!
    Mesh cube;
    cube.vertices.resize(8);
    cube.vertices[0].position = { 0, 0, 0 };
    cube.vertices[1].position = { 0, 1, 0 };
    cube.vertices[2].position = { 1, 0, 0 };
    cube.vertices[3].position = { 1, 1, 0 };
    cube.vertices[4].position = { 1, 1, 1 };
    cube.vertices[5].position = { 0, 1, 1 };
    cube.vertices[6].position = { 1, 1, 0 };
    cube.vertices[7].position = { 1, 0, 1 };
    
    cube.indices.resize(4);
    cube.indices[0] = 0;
    cube.indices[1] = 1;
    cube.indices[2] = 2;
    cube.indices[3] = 3;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    for (auto v : cube.vertices)
    {
        v.color = { dis(gen), dis(gen), dis(gen), 1.0f };
    }

    meshes["default_cube"] = cube;
    
}

void AssetManager::initDefaultTexture() {

}
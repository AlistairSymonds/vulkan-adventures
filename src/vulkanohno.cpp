#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include "VkBootstrap.h"
#include "vulkanohno.h"
#include "PipelineBuilder.h"


#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout << (__FILE__) << ":" << (__LINE__) << " Detected Vulkan error: " << string_VkResult(err) << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

VulkanOhNo::VulkanOhNo()
{
    bQuitRequested = false;
}

VulkanOhNo::~VulkanOhNo()
{

}

void VulkanOhNo::IncrementPipeline()
{
    if (current_pipe_idx >= vkPipelines.size()-1)
    {
        current_pipe_idx = 0;
    }
    else {
        current_pipe_idx++;
    }

    std::cout << "Select vkPipeline: " << vkPipelines[current_pipe_idx].first << std::endl;
}

int VulkanOhNo::init()
{   

    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    //create blank SDL window for our application
    _window = SDL_CreateWindow(
        "Vulkan Adventures", //window title
        SDL_WINDOWPOS_UNDEFINED, //window position x (don't care)
        SDL_WINDOWPOS_UNDEFINED, //window position y (don't care)
        _windowExtent.width,  //window width in pixels
        _windowExtent.height, //window height in pixels
        window_flags
    );

    init_vk();
    init_swapchain();
    init_commands();
    init_dynamic_rendering();
    init_sync();
    init_pipelines();
    load_meshes();
    _isInitialized = true;

    return true;
}

int VulkanOhNo::run() {
    draw();
    return 0;
}

int VulkanOhNo::draw() {
    //before we wait for the previous frame to finish, calculate everything we need to do locally on the CPU
    glm::mat4 view = glm::translate(glm::mat4(1.f), cam.pos);
    //camera projection
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;
    //model rotation
    glm::mat4 model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));

    //calculate final mesh matrix
    glm::mat4 mesh_matrix = projection * view * model;

    MeshPushConstants constants;
    constants.render_matrix = mesh_matrix;



    //Previous frame waiting and finishing
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &renderFence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, presentSemaphore, nullptr, &swapchainImageIndex));
    default_colour_attach_info.imageView = _swapchainImageViews[swapchainImageIndex];
    default_colour_attach_info.clearValue.color.float32[2] = abs(sin(_frameNumber / 120.f));

    //new frame stuff
    VkGraphicsPipelineCreateInfo pci;
    pci = {};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    
    VK_CHECK(vkResetCommandBuffer(cmdBuffer, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;

    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo));


    pre_draw_image_memory_barrier.image = _swapchainImages[swapchainImageIndex];
    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &pre_draw_image_memory_barrier // pImageMemoryBarriers
    );

    vkCmdBeginRendering(cmdBuffer, &default_ri);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelines[current_pipe_idx].second);
    if (vkPipelines[current_pipe_idx].first == "Mesh")
    {
        vkCmdPushConstants(cmdBuffer, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshes[0].vertexBuffer.buf, &offset);

    vkCmdDraw(cmdBuffer, meshes[0].vertices.size(), 1, 0, 0);
    vkCmdEndRendering(cmdBuffer);


    post_draw_image_memory_barrier.image = _swapchainImages[swapchainImageIndex];
    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &post_draw_image_memory_barrier // pImageMemoryBarriers
    );


    VK_CHECK(vkEndCommandBuffer(cmdBuffer));


    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;
    //wait for the output into the colour attachment to be ready
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &presentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderSemaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmdBuffer;

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(gfx_q, 1, &submit, renderFence));


    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(gfx_q, &presentInfo));

    //increase the number of frames drawn
    _frameNumber++;

    
}

void VulkanOhNo::cleanup() {
    if (_isInitialized) {
        //make sure the GPU has stopped doing its things
        vkWaitForFences(device, 1, &renderFence, true, 1000000000);

        cleanup_queue.flush();

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }
}

int VulkanOhNo::init_vk()
{

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan Adventures")
        .require_api_version(1,3)
        .request_validation_layers()
        .use_default_debug_messenger()
        .build();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
        return false;
    }
    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    

    SDL_Vulkan_CreateSurface(_window, vkb_inst.instance, &_surface);

    VkPhysicalDeviceVulkan13Features features_13;
    features_13 = {};
    features_13.dynamicRendering = true;
    
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_ret = selector.set_surface(_surface)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
        .set_minimum_version(1, 3)
        .set_required_features_13(features_13)
        .add_required_extension("VK_KHR_pipeline_library")
        .add_required_extension("VK_KHR_deferred_host_operations")
        .add_required_extension("VK_KHR_acceleration_structure")
        .add_required_extension("VK_KHR_ray_tracing_pipeline")
        .add_required_extension("VK_KHR_ray_query")
        .require_dedicated_transfer_queue()
        .select();
    if (!phys_ret) {
        std::cerr << "Failed to select Vulkan Physical Device. Error: " << phys_ret.error().message() << "\n";
        return false;
    }
    _chosenGPU = phys_ret.value();

    vkb::DeviceBuilder device_builder(*phys_ret);
    // automatically propagate needed data from instance & physical device
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create Vulkan device. Error: " << dev_ret.error().message() << "\n";
        return false;
    }
    vkb::Device vkb_device = dev_ret.value();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkb_device.device;

    // Get the graphics queue with a helper function
    auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        std::cerr << "Failed to get graphics queue. Error: " << graphics_queue_ret.error().message() << "\n";
        return false;
    }
    gfx_q = graphics_queue_ret.value();
    gfx_q_index = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = _chosenGPU;
    info.device = device;
    info.instance = _instance;
    vmaCreateAllocator(&info, &allocator);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(_chosenGPU, &props);
    std::cout << "Vulkan initialised on: " << props.deviceName << std::endl;
}

int VulkanOhNo::init_swapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, device, _surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .use_default_format_selection()
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(_windowExtent.width, _windowExtent.height)
        .build()
        .value();

    //store swapchain and its related images
    swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

    _swapchainImageFormat = vkbSwapchain.image_format;

    for (size_t i = 0; i < _swapchainImages.size(); i++)
    {
        cleanup_queue.push_function([=]() {
            vkDestroyImageView(device, _swapchainImageViews[i], nullptr);
        });
    }

    cleanup_queue.push_function([=]() {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    });
    return 0;
}

void VulkanOhNo::init_commands()
{
    //create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(gfx_q_index);
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &cmdPool));

    VkCommandBufferAllocateInfo allocInfo = vkinit::command_buffer_allocate_info(cmdPool);
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

    cleanup_queue.push_function([=]() {
        vkDestroyCommandPool(device, cmdPool, nullptr);
    });
}

void VulkanOhNo::init_dynamic_rendering()
{

    default_colour_attach_info = {};
    default_colour_attach_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    default_colour_attach_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    default_colour_attach_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    default_colour_attach_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
    
    val.color.float32[0] = 1.0;
    val.color.float32[1] = 1.0;
    val.color.float32[2] = 1.0;
    val.color.float32[3] = 1.0;
    val.depthStencil = {};
    default_colour_attach_info.clearValue = val;

    default_ri = {};
    default_ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    default_ri.colorAttachmentCount = 1;
    default_ri.layerCount = 1;
    default_ri.pColorAttachments =  &default_colour_attach_info;
    default_ri.renderArea.extent = _windowExtent;
    
    pre_draw_image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .image = _swapchainImages[0],
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
    };

    post_draw_image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .image = _swapchainImages[0],
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
    };
}

void VulkanOhNo::init_sync() {
    VkFenceCreateInfo fi;
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.pNext = NULL;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(device, &fi, nullptr, &renderFence));
    cleanup_queue.push_function([=]() {
        vkDestroyFence(device, renderFence, nullptr);
    });

    //for the semaphores we don't need any flags
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));

    cleanup_queue.push_function([=](){
        vkDestroySemaphore(device, presentSemaphore, nullptr);
        vkDestroySemaphore(device, renderSemaphore, nullptr);
    });
}

bool VulkanOhNo::load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule)
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

void VulkanOhNo::load_all_shader_modules() {
    const std::filesystem::path shader_dir("shaders");

    std::cout << "Loading .spv from:" << std::endl;
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
}


void VulkanOhNo::init_pipelines()
{

    load_all_shader_modules();
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    //use the layout then shove in the tripipe's layout
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &trianglePipelineLayout));

    PipelineBuilder pb;
    pb._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, shader_modules["basic.vert"])
    );

    pb._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, shader_modules["basic.frag"])
    );

    pb._vertexInputInfo = vkinit::vertx_input_state_create_info();
    pb._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pb._viewport.x = 0.0f;
    pb._viewport.y = 0.0f;
    pb._viewport.width = (float)_windowExtent.width;
    pb._viewport.height = (float)_windowExtent.height;
    pb._viewport.minDepth = 0.0f;
    pb._viewport.maxDepth = 1.0f;

    pb._scissor.offset = { 0, 0 };
    pb._scissor.extent = _windowExtent;


    pb._rasterizer = vkinit::raster_state_create_info();
    pb._multisampling = vkinit::multisampling_state_create_info();
    pb._colorBlendAttachment = vkinit::color_blend_attachment_state();

    pb._pipelineLayout = trianglePipelineLayout;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &_swapchainImageFormat,
    };

    auto trianglePipe = pb.build_pipeline(device, pipeline_rendering_create_info);
    vkPipelines.push_back({ "Static tri", trianglePipe });

    //Now build mesh pipeline
    VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VkPushConstantRange push_constant;
    push_constant.size = sizeof(MeshPushConstants);
    push_constant.offset = 0;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;
    
    VK_CHECK(vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &meshPipelineLayout));
    pb._pipelineLayout = meshPipelineLayout;

    auto vertexDesc = Vertex::get_vertex_description();
    pb._vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();
    pb._vertexInputInfo.vertexAttributeDescriptionCount = vertexDesc.attributes.size();


    pb._vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
    pb._vertexInputInfo.vertexBindingDescriptionCount = vertexDesc.bindings.size();
    pb._shaderStages.clear();
    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, shader_modules["mvp_mesh.vert"]));
    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, shader_modules["basic.frag"]));
    
    auto mvpMeshPipe = pb.build_pipeline(device, pipeline_rendering_create_info);
    vkPipelines.push_back({ "Mesh", mvpMeshPipe });

    //After all pipelines have been created, do the cleanup
    for (auto p : vkPipelines) {
        cleanup_queue.push_function([=]() {
            vkDestroyPipeline(device, p.second, nullptr);
        });
    }

    cleanup_queue.push_function([=]() {
        vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
    });
   

    for (auto sm : shader_modules) {
        vkDestroyShaderModule(device, sm.second, nullptr);
    }
}

void VulkanOhNo::upload_mesh(Mesh& m) {
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

void VulkanOhNo::load_meshes()
{
    Mesh tri;
    tri.vertices.resize(3);

    //vertex positions
    tri.vertices[0].position = { 1.f, 1.f, 1.0f };
    tri.vertices[1].position = { -1.f, 1.f, 0.0f };
    tri.vertices[2].position = { 0.f,-1.f, 0.0f };

    //vertex colors, all green
    tri.vertices[0].color = { 1.f, 1.f, 0.0f }; 
    tri.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
    tri.vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green

    meshes.push_back(tri);

    for (auto &m : meshes)
    {
        upload_mesh(m);
    }
}

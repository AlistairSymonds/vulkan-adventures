#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <glm/gtx/string_cast.hpp>
#include <vk_mem_alloc.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include "VkBootstrap.h"
#include "vulkanohno.h"
#include "PipelineBuilder.h"
#include "RasterEngine.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;


VulkanOhNo::VulkanOhNo()
{
    bQuitRequested = false;
}

VulkanOhNo::~VulkanOhNo()
{

}

void VulkanOhNo::IncrementRenderEngine()
{
    if (current_engine_idx >= renderEngines.size()-1)
    {
        current_engine_idx = 0;
    }
    else {
        current_engine_idx++;
    }

    std::cout << "Select engine: " << renderEngines[current_engine_idx]->getName() << std::endl;
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
    init_swapchain_barriers();
    init_sync();
    init_asset_manager();
    init_engines();
    init_imgui();
    _isInitialized = true;

    return true;
}

//This is called exactly once per frame
int VulkanOhNo::draw(std::vector<RenderObject> renderObjs) {
    
    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);
    ImGui::NewFrame();

    //some imgui UI to test
    if (ImGui::Begin("Camera")) {
        std::string posstr = glm::to_string(cam.position);
        ImGui::Text("Position: %s", (posstr.c_str()));
        

        ImGui::End();
    }
    ImGui::Render();


    //make imgui calculate internal draw structures
    ImGui::Render();

    auto &engine = renderEngines[current_engine_idx];
    RenderEngine::RenderState state;
    cam.update();

    auto projection = glm::perspective(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 0.1f, 100000.0f);

    projection[1][1] *= -1;
    state.camProj = projection;
    state.camView = cam.getViewMatrix();


    
    //Previous frame waiting and finishing
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &renderFence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, presentSemaphore, nullptr, &swapchainImageIndex));

    
    //new frame stuff
        
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

    engine->Draw(cmdBuffer, state, renderObjs);  

    //convert format via blit
    if (engine->getOutputFormat() != _swapchainImageFormat)
    {
        VkImageBlit blitRegion = {};
        blitRegion.srcOffsets[0] = { 0, 0, 0 };
        blitRegion.srcOffsets[1] = { 1700, 900, 1 };
        blitRegion.dstOffsets[0] = { 0, 0, 0 };
        blitRegion.dstOffsets[1] = { 1700, 900, 1 };

        VkImageSubresourceLayers sub_rsrc = {};
        sub_rsrc.baseArrayLayer = 0;
        sub_rsrc.layerCount = 1;
        sub_rsrc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        sub_rsrc.mipLevel = 0;
        blitRegion.dstSubresource = sub_rsrc;
        blitRegion.srcSubresource = sub_rsrc;

        VkImageLayout renderEngineOutputLayout;
        renderEngineOutputLayout;
        vkCmdBlitImage(cmdBuffer,
            engine->getOutputImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _swapchainImages[swapchainImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blitRegion,
            VK_FILTER_NEAREST
        );
    }

    post_draw_image_memory_barrier.image = _swapchainImages[swapchainImageIndex];
    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // gigabarrier before drawing the imgui
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &post_draw_image_memory_barrier // pImageMemoryBarriers
    );

    draw_imgui(cmdBuffer, _swapchainImageViews[swapchainImageIndex]);
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

        for (auto &r : renderEngines) {
            r->cleanup();
        }
        am->cleanup();
        cleanup_queue.flush();

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }
}

VkOhNoWindow VulkanOhNo::getWindw()
{
    
    VkOhNoWindow w;
    w.h = _windowExtent.height;
    w.w = _windowExtent.width;
    w.window = _window;
    return w;
}

int VulkanOhNo::init_vk()
{
    auto system_info = vkb::SystemInfo::get_system_info().value();

    VkValidationFeatureEnableEXT en;
    
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan Adventures")
        .require_api_version(1,3)
        .enable_extension("VK_EXT_validation_features")
        //.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
        //.enable_layer("VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT")
        //.enable_layer("VALIDATION_CHECK_ENABLE_VENDOR_SPECIFIC_AMD")
        .request_validation_layers()
        .use_default_debug_messenger()
        .build();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
        return false;
    }
    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, vkb_inst.instance, &_surface);

    
    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(_instance, &deviceCount, NULL);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data());

    for (auto d : physicalDevices)
    {   
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);
        cout << props.deviceName << std::endl;
    }


    VkPhysicalDeviceVulkan13Features features_13;
    features_13 = {};
    features_13.dynamicRendering = true;
    features_13.synchronization2 = true;
    VkPhysicalDeviceVulkan12Features features_12;
    features_12 = {};
    features_12.bufferDeviceAddress = true;
    
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    selector.set_surface(_surface)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .set_minimum_version(1, 3)
        .set_required_features_13(features_13)
        .set_required_features_12(features_12)
        /*
        //Render doc nolikey :(
        .add_required_extension("VK_KHR_pipeline_library")
        .add_required_extension("VK_KHR_deferred_host_operations")
        .add_required_extension("VK_KHR_acceleration_structure")
        .add_required_extension("VK_KHR_ray_tracing_pipeline")
        .add_required_extension("VK_KHR_ray_query")
        */
        .require_dedicated_transfer_queue();
        
    auto phys_ret = selector.select();
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
    info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
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
        .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
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

    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &_immCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &_immCommandBuffer));

    cleanup_queue.push_function([=]() {
        vkDestroyCommandPool(device, _immCommandPool, nullptr);
        });
}

void VulkanOhNo::init_swapchain_barriers()
{
    
    pre_draw_image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

void VulkanOhNo::init_asset_manager()
{
    am = make_shared<AssetManager>(_instance, device, gfx_q, gfx_q_index, allocator);
    am->loadAssets();
}


void VulkanOhNo::init_engines()
{   
    
    unique_ptr<RasterEngine> re_ptr;
    re_ptr = std::make_unique<RasterEngine>(device, _instance, _windowExtent, am, allocator);
    re_ptr->init();
    renderEngines.push_back(std::move(re_ptr));

    std::cout << "Initialised the following engines:" << std::endl;
    for (auto &e : renderEngines)
    {
        std::cout << "  " << e->getName() << std::endl;
    }
    std::cout << "Current engine: " << renderEngines[current_engine_idx]->getName() << std::endl;

}

void VulkanOhNo::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VK_CHECK(vkResetFences(device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    auto cmd = _immCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(gfx_q, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(device, 1, &_immFence, true, 9999999999));


}

void VulkanOhNo::init_imgui()
{
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));
    

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    // this initializes imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(_window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _chosenGPU;
    init_info.Device = device;
    init_info.Queue = gfx_q;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.ColorAttachmentFormat = _swapchainImageFormat;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);
    ImGui_ImplVulkan_CreateFontsTexture();
    // add the destroy the imgui created structures
    cleanup_queue.push_function([=]() {
        //vkDestroyDescriptorPool(device, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        });
}

void VulkanOhNo::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(_windowExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}
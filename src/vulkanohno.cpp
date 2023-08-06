#include <iostream>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include "VkBootstrap.h"
#include "vulkanohno.h"


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
    _isInitialized = true;

    return true;
}

int VulkanOhNo::run() {
    draw();
    return 0;
}

int VulkanOhNo::draw() {
    //Previous frame waiting and finishing
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &renderFence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, presentSemaphore, nullptr, &swapchainImageIndex));

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
    vkCmdBeginRendering(cmdBuffer, &default_ri);
    //no triangles yet!
    vkCmdEndRendering(cmdBuffer);
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


    

    
}

void VulkanOhNo::cleanup() {
    if (_isInitialized) {
        SDL_DestroyWindow(_window);
        vkDestroyCommandPool(device, cmdPool, nullptr);
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
}

void VulkanOhNo::init_dynamic_rendering()
{

    default_colour_attach_info = {};
    default_colour_attach_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    default_colour_attach_info.imageView = _swapchainImageViews[0];
    default_colour_attach_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    default_colour_attach_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    default_colour_attach_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
    VkClearValue val;
    val.color = { 1.0, 0.5, 0.0, 1.0 };
    default_colour_attach_info.clearValue = val;

    default_ri = {};
    default_ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    default_ri.colorAttachmentCount = 1;
    default_ri.pColorAttachments =  &default_colour_attach_info;
    default_ri.renderArea.extent = _windowExtent;
    

}

void VulkanOhNo::init_sync() {
    VkFenceCreateInfo fi;
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.pNext = NULL;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(device, &fi, nullptr, &renderFence));

    //for the semaphores we don't need any flags
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));
}

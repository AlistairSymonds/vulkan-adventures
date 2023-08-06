#include <iostream>
#include <vulkan/vulkan.h>

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
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
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
    _isInitialized = true;

    return true;
}

int VulkanOhNo::run() {
    draw();
    return 0;
}

int VulkanOhNo::draw() {
    return 0;
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

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_ret = selector.set_surface(_surface)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
        .set_minimum_version(1, 3)
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
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

    _swapchainImageFormat = vkbSwapchain.image_format;
    return 0;
}

void VulkanOhNo::init_commands()
{
    //create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(gfx_q_index);
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &cmdPool));

    VkCommandBufferAllocateInfo allocInfo = vkinit::command_buffer_allocate_info(cmdPool);
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);
}

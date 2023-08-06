#pragma once
#include <vulkan/Vulkan.h>
#include <vector>
#include "Camera.h"

class VulkanOhNo
{
public:
	VulkanOhNo();
	~VulkanOhNo();

	Camera cam;
	bool bQuitRequested;

	int init();


	int run();
	int draw();
	void cleanup();

private:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	int init_vk();
	VkDevice device;
	VkQueue gfx_q;
	VkInstance _instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
	VkSurfaceKHR _surface; // Vulkan window surface


	VkSwapchainKHR _swapchain; // from other articles
	VkFormat _swapchainImageFormat; // image format expected by the windowing system
	std::vector<VkImage> _swapchainImages; //array of images from the swapchain
	std::vector<VkImageView> _swapchainImageViews; 	//array of image-views from the swapchain


	int init_swapchain();
};
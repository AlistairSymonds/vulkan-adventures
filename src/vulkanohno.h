#pragma once
#include <vulkan/Vulkan.h>
#include <vector>
#include <unordered_set>
#include <filesystem>
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

	VkDevice device;
	VkQueue gfx_q;
	uint32_t gfx_q_index;
	VkInstance _instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
	VkSurfaceKHR _surface; // Vulkan window surface
	int init_vk();

	VkSwapchainKHR swapchain; // from other articles
	VkFormat _swapchainImageFormat; // image format expected by the windowing system
	std::vector<VkImage> _swapchainImages; //array of images from the swapchain
	std::vector<VkImageView> _swapchainImageViews; 	//array of image-views from the swapchain
	int init_swapchain();

	VkCommandPool cmdPool;
	VkCommandBuffer cmdBuffer;
	void init_commands();

	VkRenderingInfo default_ri;
	VkRenderingAttachmentInfo default_colour_attach_info;
	VkClearValue val;
	VkImageMemoryBarrier pre_draw_image_memory_barrier, post_draw_image_memory_barrier;
	void init_dynamic_rendering();

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;
	void init_sync();

	bool load_shader_module(std::filesystem::path shader_path, VkShaderModule* outShaderModule);
	VkShaderModule triangleFragmentShader, triangleVertexShader;
	void load_shaders();

	VkPipelineLayout trianglePipelineLayout;
	VkPipeline trianglePipe;
	void init_pipelines();
};
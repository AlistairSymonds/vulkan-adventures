#pragma once
#include <vulkan/Vulkan.h>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <vk_mem_alloc.h>
#include <memory>
#include "DeletionQueue.h"
#include "Camera.h"
#include "Mesh.h"
#include "AssetManager.h"
#include "RenderEngine.h"
#include "RenderObject.h"


struct VkOhNoWindow
{
	int w;
	int h;
	SDL_Window* window;
};

class VulkanOhNo
{
public:
	VulkanOhNo();
	~VulkanOhNo();

	Camera cam;
	bool bQuitRequested;

	void IncrementRenderEngine();
	int init();

	int draw(std::vector<RenderObject> renderObjs);
	void cleanup();

	VkOhNoWindow getWindw();

	// immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

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


	VkImageMemoryBarrier pre_draw_image_memory_barrier, post_draw_image_memory_barrier;
	void init_swapchain_barriers();

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;
	void init_sync();

	std::shared_ptr<AssetManager> am;
	void init_asset_manager();

	uint32_t current_engine_idx = 0;
	std::vector<std::unique_ptr<RenderEngine>> renderEngines;
	void init_engines();


	VmaAllocator allocator;

	DeletionQueue cleanup_queue;
	void init_imgui();


};
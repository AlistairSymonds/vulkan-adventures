#pragma once
#include <vulkan/Vulkan.h>
#include <vector>
#include <deque>
#include <functional>
#include <unordered_set>
#include <filesystem>
#include <vk_mem_alloc.h>
#include <memory>
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

	void IncrementPipeline();
	int init();

	
	int run();
	int draw();
	void cleanup();

	VkOhNoWindow getWindw();
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
	VkRenderingAttachmentInfo default_depth_attach_info;
	VkClearValue val;
	VkImageMemoryBarrier pre_draw_image_memory_barrier, post_draw_image_memory_barrier;
	void init_dynamic_rendering();

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;
	void init_sync();

	std::shared_ptr<AssetManager> am;
	void init_asset_manager();

	std::unordered_map<std::string, VkPipelineLayout> vkPipelineLayouts;
	std::vector<std::pair<std::string, VkPipeline>> vkPipelines;

	std::vector<std::unique_ptr<RenderEngine>> renderEngines;
	void init_engines();

	uint32_t current_pipe_idx = 0;
	void init_pipelines();

	VmaAllocator allocator;

	void upload_mesh(Mesh& mesh);
	std::unordered_map<std::string, Mesh> meshes;
	void load_meshes();

	struct MeshPushConstants{
		glm::vec4 data;
		glm::mat4 render_matrix;
	};

	struct DeletionQueue {
		std::deque<std::function<void()>> deletors;
		void push_function(std::function<void()>&& function) {
			deletors.push_back(function);
		}

		void flush() {
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)(); //deference function, then call it
			}

			deletors.clear();
		}
	};

	DeletionQueue cleanup_queue;
};
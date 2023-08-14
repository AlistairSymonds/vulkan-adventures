#pragma once
#include <memory>
#include <vma/vk_mem_alloc.h>
#include "DeletionQueue.h"
#include "RenderEngine.h"
#include "AssetManager.h"

class RasterEngine : public RenderEngine
{
public:
	RasterEngine(VkDevice dev, VkExtent2D windowExt, std::shared_ptr<AssetManager> manager, VmaAllocator vmalloc);
	std::string getName();
	~RasterEngine();
	void init();
	void cleanup();

	void Draw(VkCommandBuffer buf, RenderState state, std::vector<RenderObject> objs);
	VkFormat getOutputFormat();
	VkImage getOutputImage();


private:
	std::shared_ptr<AssetManager> am;
	VmaAllocator allocator; //already an opaque pointer
	VkDevice device;
	VkExtent2D windowExtent;
	DeletionQueue cleanup_queue;

	VkRenderingInfo default_ri;
	VkRenderingAttachmentInfo default_colour_attach_info;
	VkRenderingAttachmentInfo default_depth_attach_info;
	void init_dynamic_rendering();

	void init_pipelines();

	void init_materials();

	VkFormat outputFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	VkImage outputImage;
	VmaAllocation outputImageAllocation;
	VkImageView outputImageView;
	VkImageSubresource outputImageSubresource;
	VkImageMemoryBarrier pre_draw_image_memory_barrier, post_draw_image_memory_barrier;
	
	void init_output();
	void init_draw_barriers();
	struct Material
	{
		VkPipelineLayout layout;
		VkPipeline pipe;
	};

	std::unordered_map <std::string, Material> materials;
};

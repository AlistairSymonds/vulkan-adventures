#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct AllocatedBuffer
{
	VkBuffer buf;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};
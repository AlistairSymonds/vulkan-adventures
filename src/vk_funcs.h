#pragma once
#include <vk_mem_alloc.h>
#include "vk_types.h"
#include "vk_initializers.h"


namespace vkfuncs {
    AllocatedBuffer create_buffer(VmaAllocator vma, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
}
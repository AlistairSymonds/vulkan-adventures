#include "RasterEngine.h"
#include "RasterEngine.h"
#include "RasterEngine.h"
#include <vulkan/vulkan.h>
#include "RasterEngine.h"

RasterEngine::RasterEngine(VkDevice dev, std::shared_ptr<AssetManager> manager) {
	am = manager;
	device = dev;
}

std::string RasterEngine::getName()
{
	return "Raster";
}

RasterEngine::~RasterEngine()
{
}

void RasterEngine::cleanup() {

}

void RasterEngine::init()
{
}


void RasterEngine::Draw(VkCommandBuffer buf, std::vector<RenderObject> objs)
{
}

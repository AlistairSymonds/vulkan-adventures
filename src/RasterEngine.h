#pragma once
#include <memory>
#include "RenderEngine.h"
#include "AssetManager.h"
class RasterEngine : public RenderEngine
{
public:
	RasterEngine(VkDevice dev, std::shared_ptr<AssetManager> manager);
	std::string getName();
	~RasterEngine();
	void init();
	void cleanup();

	void Draw(VkCommandBuffer buf, std::vector<RenderObject> objs);
private:
	std::shared_ptr<AssetManager> am;
	VkDevice device;

};

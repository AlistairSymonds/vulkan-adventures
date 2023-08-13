#pragma once
#include <vector>
#include <string>
#include "RenderObject.h"
class RenderEngine
{
public:
	//RenderEngine();
	//virtual ~RenderEngine() = 0;
	virtual void Draw(VkCommandBuffer buf, std::vector<RenderObject> objs) = 0;
	virtual void init() = 0;
	virtual void cleanup() = 0;
	virtual std::string getName() = 0;
private:

};

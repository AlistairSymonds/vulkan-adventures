#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/matrix.hpp>
#include "RenderObject.h"



class RenderEngine
{
public:

	struct RenderState
	{
		glm::mat4 camView;
		glm::mat4 camProj;
	};
	//RenderEngine();
	//virtual ~RenderEngine() = 0;
	virtual void Draw(VkCommandBuffer buf, RenderState state, std::vector<RenderObject> objs) = 0;
	virtual void init() = 0;
	virtual void cleanup() = 0;
	virtual std::string getName() = 0;
	virtual VkFormat getOutputFormat() = 0;	
	virtual VkImage getOutputImage() = 0;


private:


};

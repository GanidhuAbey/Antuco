#pragma once

#include 

#include <string>
#include <vulkan/vulkan.hpp>


namespace com {

class ApiSkyBox;

class SkyBox {
private:
	std::string image_path;
	VkImage image;
public:
	SkyBox(RenderEngine api, std::string image_path);
};

}
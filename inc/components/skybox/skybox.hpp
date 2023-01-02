#pragma once

#include <antuco_enums.hpp>
#include <string>
#include <vkwr.hpp>

namespace com {

class ApiSkyBox;

class SkyBox {
private:
	std::string image_path;
	VkImage image;
public:
	SkyBox(tuco::RenderEngine api, std::string image_path);
};

}
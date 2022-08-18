#pragma once

#include 

#include <string>


namespace com {

class ApiSkyBox;

class SkyBox {
private:
	std::string image_path;
public:
	SkyBox(RenderEngine api, std::string image_path);
};

}
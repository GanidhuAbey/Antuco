#pragma once

#include <string>

#include <bedrock/image.hpp>

namespace tuco {

class Cubemap {
private:
	br::Image cubemap;

public:
	// the provided path should be to a folder containing all the skybox images
	// images should be named such that:
	//	"nx" - negative x image
	//	"px" - positive x image
	void init(std::string path);

	br::Image& get_image() { return cubemap; }
};

}
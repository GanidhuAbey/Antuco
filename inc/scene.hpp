#pragma once

#include <bedrock/image.hpp>

#include <string>

namespace tuco
{

class SceneData
{
public:
	bool has_ibl = false;
private:
	br::Image ibl_image;
	uint32_t index;

public:
	br::Image& get_ibl() { return ibl_image; }
	void set_ibl(std::string file_path);

	void set_index(uint32_t index) { SceneData::index = index; }
	uint32_t get_index() { return index; }
};

}
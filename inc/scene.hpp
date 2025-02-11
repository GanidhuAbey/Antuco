#pragma once

#include <bedrock/image.hpp>
#include <environment.hpp>
#include <world_objects.hpp>

#include <string>
#include <unordered_map>

namespace tuco
{

class SceneData
{
public:
	bool has_ibl = false;
	bool has_skybox = false;
private:
	br::Image ibl_image;

	Environment skybox;
	GameObject skybox_model;

	std::unordered_map<ResourceCollection*, uint32_t> index_map = {};

public:
	SceneData();
	//SceneData(SceneData&) = delete; // delete copy function (should only have 1 scene anyways?)

	br::Image& get_ibl() { return ibl_image; }
	void set_ibl(std::string file_path);

	// the provided path should be to a folder containing all the skybox images
	// images should be named such that:
		//	"nx" - negative x image
		//	"px" - positive x image
	void set_skybox(std::string file_path);
	Environment& get_skybox() { return skybox; };
	GameObject& get_skybox_model() { return skybox_model; }

	void set_index(ResourceCollection* collection, uint32_t index);
	uint32_t get_index(ResourceCollection* collection);

	bool update_gpu = false;
	uint32_t ubo_offset = 0;
};

}
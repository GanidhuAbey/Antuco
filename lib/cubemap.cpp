#include <cubemap.hpp>

#include <vector>

using namespace tuco;

#define SKYBOX_NAMES {"px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png"}

void Cubemap::init(std::string file_path)
{
	std::vector<std::string> paths = SKYBOX_NAMES;
	for (int i = 0; i < 6; i++)
	{
		paths[i] = file_path + "/" + paths[i];
	}

	cubemap.init("skybox");
	cubemap.load_cubemap(paths, br::ImageFormat::RGBA_COLOR);
	cubemap.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}
#include <scene.hpp>

using namespace tuco;

void SceneData::set_ibl(std::string file_path)
{
	ibl_image.init("Ibl Image");
	ibl_image.load_image(file_path, br::ImageFormat::RGBA_COLOR, br::ImageType::Image_3D);
	ibl_image.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	has_ibl = true;
}
#include <scene.hpp>

#include <format>

#include <antuco.hpp>
#include <api_graphics.hpp>

using namespace tuco;

SceneData::SceneData()
{
	index_map.clear();
}

void SceneData::set_index(ResourceCollection* collection, uint32_t index)
{
	index_map[collection] = index;
}

void SceneData::set_ibl(std::string file_path)
{
	ibl_image.init("Ibl Image");
	ibl_image.load_image(file_path, br::ImageFormat::RGBA_COLOR, br::ImageType::Image_3D);
	ibl_image.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	has_ibl = true;
}

void SceneData::set_skybox(std::string file_path)
{
	std::string project_path = get_project_root(__FILE__);
	skybox_model.add_mesh(project_path + "/objects/antuco-files/windows/cube.glb", "skybox");
	skybox_model.scale(glm::vec3(5.0, 5.0, 5.0));

	// Skybox model
	skybox_model.buffer_index_offset = Antuco::get_engine().get_backend()->update_index_buffer(skybox_model.object_model.model_indices) / sizeof(uint32_t);
	skybox_model.buffer_vertex_offset = Antuco::get_engine().get_backend()->update_vertex_buffer(skybox_model.object_model.model_vertices) / sizeof(Vertex);

	update_gpu = true;
	has_skybox = true;
	skybox.init(file_path, &skybox_model);
}

uint32_t SceneData::get_index(ResourceCollection* collection)
{
	auto search = index_map.find(collection);
	ASSERT(search != index_map.end(), "collection does not exist in map");

	return search->second;
}
#include "api_graphics.hpp"
#include "window.hpp"
#include "api_window.hpp"
#include "logger/interface.hpp"
#include "config.hpp"

#include <glm/ext.hpp>

using namespace tuco;

GraphicsImpl::GraphicsImpl(Window* pWindow)
    : instance(pWindow->get_title(), VK_MAKE_API_VERSION(0, 1, 1, 0)),
      physical_device(instance),
      surface(instance, pWindow->pWindow->apiWindow) {
	not_created = true;
	raytracing = false; //set this as an option in the pre-configuration settings.
	oit_layers = 1;
	depth_bias_constant = 7.0f;
	depth_bias_slope = 9.0f;
	print_debug = true;
#ifdef APPLE_M1
    print_debug = false;
    raytracing = false;
#endif
	create_logical_device();
	create_command_pool();

	//graphics draw
	create_swapchain();
	//create_shadowmap_atlas();
	//create_shadowmap_transfer_buffer();
	create_depth_resources();
	create_shadowpass_resources();
    create_output_images();
    create_render_pass();
    //create_geometry_pass();
	create_shadowpass();
	create_ubo_layout();
	create_light_layout();
    create_materials_layout();
    create_materials_pool();
	create_texture_layout();
	create_shadowmap_layout();
	create_shadowmap_pool();
	create_graphics_pipeline();
	create_shadowpass_pipeline();
	create_texture_sampler();
	create_shadowmap_sampler();
	create_output_buffers();
	create_shadowpass_buffer();
	create_shadowmap_set();
	write_to_shadowmap_set();
	create_semaphores();
	create_fences();

	//create some buffers now
	create_vertex_buffer();
	create_index_buffer();
	create_uniform_buffer();

	create_ubo_pool();
	create_texture_pool();

    create_screen_pass();
    create_screen_buffer();
    create_screen_pipeline();
	create_screen_set();
}

GraphicsImpl::~GraphicsImpl() {
	destroy_draw();
	destroy_initialize();
}

void GraphicsImpl::update_camera(glm::mat4 world_to_camera, glm::mat4 projection, glm::vec4 eye) {
	//construct/update the current universal camera matrices
	camera_view = world_to_camera;
	camera_projection = projection;
    camera_pos = eye;
}

void GraphicsImpl::update_light(std::vector<DirectionalLight> lights, std::vector<int> shadow_casters) {
	light_data = lights;
	shadow_caster_indices = shadow_casters;
}

//TODO: with that most of the rendering segments are in place all thats actually left is to implement texturing, and we can begin
//bugfixing to arrive at a working version.

//NOTE: enable sync validation to check that ubo read-write hazard is not occuring
void GraphicsImpl::update_draw(std::vector<GameObject*> game_objects) {
	//now the question is what do we do here?
	//we need to create some command buffers
	//update vertex and index buffers

	//store a pointer of all the objects in the scene, in case we have to render the image again for whatever reason
	recent_objects = &game_objects;

	bool update_command_buffers = false;
	auto offset = 0;
	for (size_t i = 0; i < game_objects.size(); i++) {
        const auto& model = game_objects[i]->object_model;

		//TODO: make light buffer object a ubo with only 2 matricies for the respective light data.
		//		it will make the whole thing more elegant once we have more light data to deal with

		if (game_objects[i]->update) {
            //update the buffer data of game objects
			uint32_t index_mem = update_index_buffer(model.model_indices);
			uint32_t vertex_mem = update_vertex_buffer(model.model_vertices);

			update_command_buffers = true;
			game_objects[i]->update = false;
			create_ubo_set(static_cast<uint32_t>(model.transforms.size()));	
			write_to_ubo();
			create_light_set(static_cast<uint32_t>(model.transforms.size()));
            
			//TODO: for some reason model_meshes or object_model does not exist
			//      and the program crashes when it attempts to access the data here.
			auto primitives = model.primitives;
			//create texture data
			if (model.model_images.size() > 0) create_texture_set(model.model_images.size());
			if (model.model_materials.size() > 0) {
				create_materials_set(model.model_materials.size());
				write_to_materials(model.model_materials.size());
			}
			for (auto& prim : primitives) {
				if (prim.mat_index >= 0) {
					auto material = model.model_materials[prim.mat_index];
					update_materials(
						mat_offsets[i][prim.mat_index],
						material);
				}

				//create vulkan image
				//why is textures even a vector???
                if (0 < prim.image_index && prim.image_index < model.model_images.size()) {
					create_vulkan_image(
						model.model_images[prim.image_index], 
						i, 
						prim.image_index);
                }
				//TODO: create alternative for when primitive does not have image.
			}
		}

		//update buffer data (for representing object information in shader)
		auto ubo = UniformBufferObject{};
		ubo.worldToCamera = camera_view;
		ubo.projection = camera_projection;

		auto lbo = UniformBufferObject{};
		lbo.worldToCamera = light_data[0].world_to_light;
		lbo.projection = light_data[0].perspective;
		for (size_t j = 0; j < model.transforms.size(); j++) {
			ubo.modelToWorld = game_objects[i]->transform * model.transforms[j];
			lbo.modelToWorld = game_objects[i]->transform * model.transforms[j];
			update_uniform_buffer(ubo_offsets[offset + j], ubo);
			//update light data (used for generating shadow map)	
			update_uniform_buffer(light_offsets[offset + j], lbo);
		}
		offset += model.transforms.size();

        

	}

	if (!update_command_buffers) {
		for (size_t i = 0; i < MAX_SHADOW_CASTERS; i++) {
			if (light_data[shadow_caster_indices[i]].generate_shadows) {
				update_command_buffers = true;
				break;
			}
		}
	}


	if (update_command_buffers) {
		//free command buffers first?
		free_command_buffers();
		create_command_buffers(game_objects);
		update_command_buffers = false;
	}

	draw_frame();
}

#include "api_graphics.hpp"
#include "window.hpp"
#include "api_window.hpp"
#include "logger/interface.hpp"
#include "config.hpp"

#include <glm/ext.hpp>

using namespace tuco;

GraphicsImpl::GraphicsImpl(Window* pWindow) {
	not_created = true;
	raytracing = false; //set this as an option in the pre-configuration settings.
	oit_layers = 1;
	depth_bias_constant = 7.0f;
	depth_bias_slope = 9.0f;
#ifdef APPLE_M1
    raytracing = false;
#endif
	create_instance(pWindow->get_title());
	pWindow->pWindow->create_vulkan_surface(instance, &surface); //it aint pretty, but it'll get er done.
	pick_physical_device();
	create_logical_device();
	create_command_pool();

	//graphics draw
	create_swapchain();
	//create_shadowmap_atlas();
	//create_shadowmap_transfer_buffer();
	create_depth_resources();
	create_shadowpass_resources();
    create_image_layers();
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
	create_swapchain_buffers();
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

void GraphicsImpl::update_light(std::vector<Light*> lights, std::vector<int> shadow_casters) {
	light_data = lights;
	shadow_caster_indices = shadow_casters;
}

//TODO: with that most of the rendering segments are in place all thats actually left is to implement texturing, and we can begin
//bugfixing to arrive at a working version.
void GraphicsImpl::update_draw(std::vector<GameObject*> game_objects) {
	//now the question is what do we do here?
	//we need to create some command buffers
	//update vertex and index buffers

	//store a pointer of all the objects in the scene, in case we have to render the image again for whatever reason
	recent_objects = &game_objects;

	bool update_command_buffers = false;
	for (size_t i = 0; i < game_objects.size(); i++) {

		//create ubo data
		UniformBufferObject ubo;

		ubo.modelToWorld = game_objects[i]->transform;
		ubo.worldToCamera = camera_view;
		ubo.projection = camera_projection;

		//TODO: make light buffer object a ubo with only 2 matricies for the respective light data.
		//		it will make the whole thing more elegant once we have more light data to deal with
		UniformBufferObject lbo;
		lbo.modelToWorld = game_objects[i]->transform;
		lbo.worldToCamera = light_data[0]->world_to_light;
		lbo.projection = light_data[0]->perspective;

		if (game_objects[i]->update) {
			update_command_buffers = true;
			game_objects[i]->update = false;
			create_ubo_set();	
			write_to_ubo();
			//will have to dynamically update the orientation to make the cross product returns a proper value 
			create_light_set(lbo);
			//TODO: for some reason model_meshes or object_model does not exist
			//      and the program crashes when it attempts to access the data here.
			std::vector<Mesh*> meshes = game_objects[i]->object_model.model_meshes;
			//create texture data
			create_texture_set(meshes.size());

            create_materials_set(meshes.size());
            write_to_materials();

			for (size_t j = 0; j < meshes.size(); j++) {
				//for now lets just assume this works so we can deal with the other errors...
				uint32_t index_mem = update_vertex_buffer(meshes[j]->vertices);
				uint32_t vertices_mem = update_index_buffer(meshes[j]->indices);

                meshes[j]->index_mem = index_mem;
                meshes[j]->vertex_mem = vertices_mem;

                update_materials(mat_offsets[i][j], meshes[j]->mat_data);

				//create vulkan image
				//why is textures even a vector???
                if (meshes[j]->mat_data.texture_opacity.r > 0) {
                    create_texture_image(meshes[j]->textures[0], i, j);
                } else {
                    create_empty_image(i, j);
                }
			}
		}
		
		//update buffer data (for representing object information in shader)
		update_uniform_buffer(ubo_offsets[i], ubo);
		//update light data (used for generating shadow map)	
		update_uniform_buffer(light_offsets[i], lbo);
	}

	if (!update_command_buffers) {
		for (size_t i = 0; i < MAX_SHADOW_CASTERS; i++) {
			//TODO: could apply some sort of sorting to light_data to add an order of importance or let the user choose what sources
			//		could be possibly cast lights, etc.
			if (light_data[shadow_caster_indices[i]]->generate_shadows) {
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

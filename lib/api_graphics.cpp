#include "api_graphics.hpp"
#include "window.hpp"
#include "api_window.hpp"

using namespace tuco;

GraphicsImpl::GraphicsImpl(Window* pWindow) {
	create_instance(pWindow->get_title());
	pWindow->pWindow->create_vulkan_surface(instance, &surface); //it aint pretty, but it'll get er done.
	pick_physical_device();
	create_logical_device();
	create_command_pool();

	//graphics draw
	create_swapchain();
	create_depth_resources();
	create_colour_image_views();
	create_render_pass();
	create_ubo_layout();
	create_texture_layout();
	create_graphics_pipeline();
	create_texture_sampler();
	create_frame_buffers();
	create_semaphores();
	create_fences();

	//create some buffers now
	create_vertex_buffer();
	create_index_buffer();
	create_uniform_buffer();
}

GraphicsImpl::~GraphicsImpl() {
	destroy_draw();
	destroy_initialize();
}

void GraphicsImpl::update_camera(glm::mat4 world_to_camera, glm::mat4 projection) {
	//construct/update the current universal camera matrices
	camera_view = world_to_camera;
	camera_projection = projection;
}

void GraphicsImpl::update_light(glm::vec4 color, glm::vec4 position) {
	light.color = color;
	light.position = position;
}


//TODO: with that most of the rendering segments are in place all thats actually left is to implement texturing, and we can begin
//bugfixing to arrive at a working version.
void GraphicsImpl::update_draw(std::vector<GameObject*> game_objects) {
	//now the question is what do we do here?
	//we need to create some command buffers
	//update vertex and index buffers

	for (size_t i = 0; i < game_objects.size(); i++) {
		//create ubo data
		UniformBufferObject ubo;

		ubo.modelToWorld = game_objects[i]->transform;
		ubo.worldToCamera = camera_view;
		ubo.projection = camera_projection;

		if (game_objects[i]->update) {
			game_objects[i]->back_end_data = create_ubo_pool();
			create_ubo_set();
			
			write_to_ubo();

			ubo_offsets.push_back(uniform_buffer.offset);
			uniform_buffer.allocate = false;

			//TODO: for some reason model_meshes or object_model does not exist
			//      and the program crashes when it attempts to access the data here.
			std::vector<Mesh*> meshes = game_objects[i]->object_model.model_meshes;
			//create texture data
			create_texture_pool(meshes.size());
			create_texture_set(meshes.size());

			for (size_t j = 0; j < meshes.size(); j++) {
				//for now lets just assume this works so we can deal with the other errors...
				for (size_t l = 0; l < meshes[j]->indices.size(); l++) {
					printf("index_data: | %u |", meshes[j]->indices[l]);
				}
				printf("\n");
				update_vertex_buffer(meshes[j]->vertices);
				update_index_buffer(meshes[j]->indices);

				//create vulkan image
				//why is textures even a vector???
				create_texture_image(meshes[j]->textures[0], i, j);
			}
			create_command_buffers(game_objects);

		}
		
		update_uniform_buffer(ubo_offsets[i], ubo);

		game_objects[i]->update = false;

		//actually drawing the frame
		draw_frame();
	}
}
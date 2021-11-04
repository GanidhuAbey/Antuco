#include "api_graphics.hpp"
#include "window.hpp"
#include "api_window.hpp"

#include <glm/ext.hpp>

using namespace tuco;

GraphicsImpl::GraphicsImpl(Window* pWindow) {
	not_created = true;

	create_instance(pWindow->get_title());
	pWindow->pWindow->create_vulkan_surface(instance, &surface); //it aint pretty, but it'll get er done.
	pick_physical_device();
	create_logical_device();
	create_command_pool();

	//graphics draw
	create_swapchain();
	create_depth_resources();
	create_shadowpass_resources();
	create_colour_image_views();
	create_render_pass();
	create_shadowpass();
	create_ubo_layout();
	create_light_layout();
	create_texture_layout();
	create_shadowmap_layout();
	create_shadowmap_pool();
	create_graphics_pipeline();
	create_shadowpass_pipeline();
	create_texture_sampler();
	create_shadowmap_sampler();
	create_frame_buffers();
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

void GraphicsImpl::update_camera(glm::mat4 world_to_camera, glm::mat4 projection) {
	//construct/update the current universal camera matrices
	camera_view = world_to_camera;
	camera_projection = projection;
}

void GraphicsImpl::update_light(glm::vec4 color, glm::vec4 position, glm::vec3 point_of_focus, std::vector<GameObject*> game_objects) {
	light.color = color;
	light.position = position;
}

//TODO: with that most of the rendering segments are in place all thats actually left is to implement texturing, and we can begin
//bugfixing to arrive at a working version.
void GraphicsImpl::update_draw(std::vector<GameObject*> game_objects) {
	//now the question is what do we do here?
	//we need to create some command buffers
	//update vertex and index buffers

	bool update_command_buffers = false;
	for (size_t i = 0; i < game_objects.size(); i++) {

		//create ubo data
		UniformBufferObject ubo;

		ubo.modelToWorld = game_objects[i]->transform;
		ubo.worldToCamera = camera_view;
		ubo.projection = camera_projection;

		UniformBufferObject lbo;

		lbo.modelToWorld = ubo.modelToWorld;
		lbo.worldToCamera = glm::lookAt(glm::vec3(light.position), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
		lbo.projection = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 96.0f); //hard coding this values may cause problems, but for now it should be fine

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

			for (size_t j = 0; j < meshes.size(); j++) {
				//for now lets just assume this works so we can deal with the other errors...
				update_vertex_buffer(meshes[j]->vertices);
				update_index_buffer(meshes[j]->indices);

				//create vulkan image
				//why is textures even a vector???
				create_texture_image(meshes[j]->textures[0], i, j);
			}
		}
		
		//update buffer data (for representing object information in shader)
		update_uniform_buffer(ubo_offsets[i], ubo);
		//update light data (used for generating shadow map)	
		update_uniform_buffer(light_offsets[i], lbo);
	}

	if (update_command_buffers) {
		create_command_buffers(game_objects);
		update_command_buffers = false;
	}

	draw_frame();
}
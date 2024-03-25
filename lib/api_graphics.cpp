#include "api_graphics.hpp"
#include "api_config.hpp"
#include "api_window.hpp"
#include "config.hpp"
#include "logger/interface.hpp"
#include "window.hpp"

#include <glm/ext.hpp>

using namespace tuco;

GraphicsImpl::GraphicsImpl(Window *pWindow)
    : instance(pWindow->get_title(), VK_MAKE_API_VERSION(0, 1, 1, 0)),
      physical_device(instance), surface(instance, pWindow->pWindow->apiWindow),
      device(physical_device, surface, false),
      swapchain(physical_device, device, surface) {

  not_created = true;
  raytracing = false; // set this as an option in the pre-configuration
                      // settings.
  oit_layers = 1;
  depth_bias_constant = 7.0f;
  depth_bias_slope = 9.0f;
#ifdef APPLE_M1
  print_debug = false;
  raytracing = false;
#endif
  command_pool = create_command_pool(device, device.get_graphics_family());

  // graphics draw
  // create_shadowmap_atlas();
  // create_shadowmap_transfer_buffer();
  create_depth_resources();
  create_shadowpass_resources();
  create_output_images();
  create_render_pass();
  // create_geometry_pass();
  create_shadowpass();
  create_ubo_layout();
  create_light_layout();
  createMaterialLayout();
  createMaterialPool();
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

  // create some buffers now
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

void GraphicsImpl::update_camera(glm::mat4 world_to_camera,
                                 glm::mat4 projection, glm::vec4 eye) {
  // construct/update the current universal camera matrices
  camera_view = world_to_camera;
  camera_projection = projection;
  camera_pos = eye;
}

void GraphicsImpl::update_light(std::vector<DirectionalLight> lights,
                                std::vector<int> shadow_casters) {
  light_data = lights;
  shadow_caster_indices = shadow_casters;
}

// NOTE: enable sync validation to check that ubo read-write hazard is not
// occuring
void GraphicsImpl::update_draw(
    std::vector<std::unique_ptr<GameObject>> &game_objects) {
  // now the question is what do we do here?
  // we need to create some command buffers
  // update vertex and index buffers

  auto offset = 0;
  for (size_t i = 0; i < game_objects.size(); i++) {
    const auto &model = game_objects[i]->object_model;

    if (game_objects[i]->update) {
      // update the buffer data of game objects
      uint32_t index_mem = update_index_buffer(model.model_indices);
      uint32_t vertex_mem = update_vertex_buffer(model.model_vertices);

      update_command_buffers = true;
      game_objects[i]->update = false;
      createUboSets(static_cast<uint32_t>(model.transforms.size()));
      write_to_ubo();
      create_light_set(static_cast<uint32_t>(model.transforms.size()));

      auto primitives = model.primitives;
      // WARNING: texture sets only created for objects with textures. this
      // means that the object index in the objects array does not always refer
      // corresponding texture set.
      if (model.model_images.size() > 0) {
        create_texture_set(model.model_images.size());
      }

      createMaterialSets(1);
      write_to_materials();

      for (auto &prim : primitives) {
        auto material =
            game_objects[i]->material; // model.model_materials[prim.mat_index];
        game_objects[i]->material.mem_access = mat_offsets.size() - 1;
        update_materials(mat_offsets[mat_offsets.size() - 1], material);

        /*
        // create vulkan image
        // why is textures even a vector???
        if (-1 < prim.image_index &&
            prim.image_index < model.model_images.size()) {
          create_vulkan_image(model.model_images[prim.image_index], i,
                              prim.image_index);
        }
        */
      }
    }

    // update buffer data (for representing object information in shader)
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
      // update light data (used for generating shadow map)
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
    // free command buffers first?
    free_command_buffers();
    create_command_buffers(game_objects);
    update_command_buffers = false;
  }

  update_command_buffers = false;

  draw_frame();
}

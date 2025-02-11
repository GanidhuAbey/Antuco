/* ---------------------- world_objects.hpp -----------------------
 * creates user-level game objects which abstract the api functions
 * away
 * ----------------------------------------------------------------
 */

#pragma once

#include "material.hpp"
#include "model.hpp"
// #include "graphics.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace tuco {

// RESTRICTION: the light has a aspect ratio of 1.0f, when generating the depth
// map from the light source, this has to be reflected in the texture size
class DirectionalLight {
  friend class Antuco;
  friend class GraphicsImpl;

private:
  glm::vec4 position;
  glm::vec4 target;
  glm::vec4 color;
  glm::vec3 orientation;

  int light_index = 0;

  glm::mat4 world_to_light;
  glm::mat4 perspective;

  bool generate_shadows = true;

private:
  DirectionalLight(glm::vec3 light_pos, glm::vec3 light_target,
                   glm::vec3 light_color,
                   glm::vec3 up = glm::vec3(0.0, 1.0, 0.0));

public:
  ~DirectionalLight();
  void update(glm::vec3 new_position,
              glm::vec3 new_direction = glm::vec3(0, 0, 0));
};

class PointLight {
private:
  glm::vec3 pos;
  glm::vec3 colour;
  // TODO: brightness?

public:
  PointLight(glm::vec3 position, glm::vec3 colour);
  ~PointLight();
};

class Camera {
  friend class Antuco;
  friend class GraphicsImpl;

public:
  glm::mat4 modelToCamera;
  glm::mat4 cameraToScreen;

  glm::vec3 point_of_focus;
  glm::vec3 pos;

  glm::vec3 orientation;

public:
    Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float yfov,
           float aspect_ratio, float near, float far);

  ~Camera();
  void update(glm::vec3 camera_pos, glm::vec3 camera_face);

private:
  glm::mat4 construct_world_to_camera(glm::vec3 eye, glm::vec3 target,
                                      glm::vec3 up);
  glm::mat4 perspective_projection(float angle, float aspect, float n, float f);
};

// TODO: separate transform data from object container.
// [TODO 08/24] - GameObject is actually a mesh component, but is named generically...
class GameObject {
  friend class Antuco;
  friend class GraphicsImpl;

private:
  glm::mat4 transform;
  bool update = true;

  // TODO: generalize the components within our container.
  // reduce coupling.
  uint32_t material_index;
  uint32_t draw_index;

  size_t back_end_data = 0;
  uint32_t changed = 0;

public:
  GameObject();
  ~GameObject();
  void add_mesh(const std::string &fileName,
                std::optional<std::string> name = std::nullopt);
  void scale(glm::vec3 scale_vector);
  void translate(glm::vec3 t);
  void set_position(glm::vec3 t);

  Material* get_material();

  uint32_t buffer_index_offset = 0;
  uint32_t buffer_vertex_offset = 0;
  Model object_model;
};

class SkyBox {
  friend class Antuco;

private:
  std::string texture_path; // a skybox contains texture.
};

} // namespace tuco

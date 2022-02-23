/* ------------------------ data_structures.hpp -------------------------
 * Various data structures, used to structure data being passed to
 * the shader.
 * ----------------------------------------------------------------------
*/
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace tuco {
struct PushFragConstant {
	glm::vec4 color;
};

struct Vertex {
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec2 tex_coord;
};

struct UniformBufferObject {
	glm::mat4 modelToWorld;
	glm::mat4 worldToCamera;
	glm::mat4 projection;
};

struct LightObject {
	glm::vec4 color;
	glm::vec4 direction;
	glm::vec4 position;
	glm::vec4 light_count;
};

struct LightBufferObject {
	glm::mat4 world_to_light;
	glm::mat4 projection;
};


/*
enum class IMAGE_FORMAT {
    DEPTH = VK_FORMAT_D16_UNORM,
    COLOR = VK_FORMAT_R8G8B8A8_SRGB,
};
*/

}

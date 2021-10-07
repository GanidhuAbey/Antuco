#pragma once

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
	glm::vec3 color;
	glm::vec3 position;
};

}

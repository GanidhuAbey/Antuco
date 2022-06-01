/* ------------------------ data_structures.hpp -------------------------
 * Various data structures, used to structure data being passed to
 * the shader.
 * ----------------------------------------------------------------------
*/
#pragma once

#include "assimp/types.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <optional>

const uint32_t MATERIALS_POOL_SIZE = 5;

namespace tuco {
struct PushFragConstant {
	glm::vec4 color;
};

class Vertex {
public:
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec2 tex_coord;

public:
	Vertex(glm::vec4 pos, glm::vec4 norm, glm::vec2 tex);
	~Vertex();

	std::string to_string();
    //restructure the data stored into a linear array of floats: [positions, normals, textures]
    std::vector<float> linearlize();
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

class MaterialsObject {
public:
    glm::vec4 texture_opacity;
    glm::vec4 ambient; //Ka
    glm::vec4 diffuse; //Kd
    glm::vec4 specular; //Ks

public:
    void init(glm::vec4 texture_opacity, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular);
    MaterialsObject();
    ~MaterialsObject();
    std::vector<float> linearlize();
};

struct Material {
    std::optional<aiString> texturePath;
    aiColor3D ambient; //Ka
    aiColor3D diffuse; //Kd
    aiColor3D specular; //Ks
    float opacity;
};

struct TransparentMesh {
	uint32_t index;
	uint32_t vertex;
	size_t k;
	size_t j;
};

/*
enum class IMAGE_FORMAT {
    DEPTH = VK_FORMAT_D16_UNORM,
    COLOR = VK_FORMAT_R8G8B8A8_SRGB,
};
*/

}

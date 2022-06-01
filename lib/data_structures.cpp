#include "data_structures.hpp"

#include "iostream"

using namespace tuco;

Vertex::Vertex(glm::vec4 pos, glm::vec4 norm, glm::vec2 tex) {
	position = pos;
	normal = norm;
	tex_coord = tex;
}

Vertex::~Vertex() {}

std::string Vertex::to_string() {
	std::string s_pos = std::to_string(position.x) + std::to_string(position.y) + std::to_string(position.z) + std::to_string(position.w);
	std::string s_norm = std::to_string(normal.x) + std::to_string(normal.y) + std::to_string(normal.z) + std::to_string(normal.w);
	std::string s_tex = std::to_string(tex_coord.x) + std::to_string(tex_coord.y);

	std::string vertex = s_pos + s_norm + s_tex;

	return vertex;
}

std::vector<float> Vertex::linearlize() {
    std::vector<float> float_list;
    float_list.push_back(position.x);
    float_list.push_back(position.y);
    float_list.push_back(position.z);

    float_list.push_back(normal.x);
    float_list.push_back(normal.y);
    float_list.push_back(normal.z);

    float_list.push_back(tex_coord.x);
    float_list.push_back(tex_coord.y);

    return float_list;
}


void MaterialsObject::init(glm::vec4 texture_opacity, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular) {
    MaterialsObject::texture_opacity = texture_opacity;
    MaterialsObject::ambient = ambient;
    MaterialsObject::diffuse = diffuse;
    MaterialsObject::specular = specular;
}

MaterialsObject::MaterialsObject() {}
MaterialsObject::~MaterialsObject() {}

std::vector<float> MaterialsObject::linearlize() {
    std::vector<float> floats;
    floats.push_back(texture_opacity.x);
    floats.push_back(texture_opacity.y);

    floats.push_back(ambient.x);
    floats.push_back(ambient.y);
    floats.push_back(ambient.z);

    floats.push_back(diffuse.x);
    floats.push_back(diffuse.y);
    floats.push_back(diffuse.z);

    floats.push_back(specular.x);
    floats.push_back(specular.y);
    floats.push_back(specular.z);

    return floats;
}

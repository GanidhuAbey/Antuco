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

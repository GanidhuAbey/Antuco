#include "data_structures.hpp"

using namespace tuco;

Vertex::Vertex(glm::vec4 pos, glm::vec4 norm, glm::vec2 tex) {
	position = pos;
	normal = norm;
	tex_coord = tex;
}

char* Vertex::to_ptr_char() {
	char c_pos = (char) position.x + (char) position.y + (char) position.z;
	char c_norm = (char)normal.x + (char)normal.y + (char)normal.z;
	char c_tex = (char)tex_coord.x + (char)tex_coord.y;

	char vertex = c_pos + c_norm + c_tex;

	return &vertex;
}
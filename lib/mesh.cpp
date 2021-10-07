#include "mesh.hpp"

using namespace tuco;


Mesh::Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices, std::vector<aiString> newTextures) {
	vertices = newVertices;
	indices = newIndices;
	textures = newTextures;
}

Mesh::~Mesh() {}
#pragma once

#include "data_structures.hpp"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <vector>

namespace tuco {

class Mesh {
	friend class Model;

public:
	//since mesh can only be accessed through model/gameObject where we already obsfucated access to meshes directly, the user won't
	//have any way to interact with these values even whilst they are in public
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<aiString> textures;

private:
	Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices, std::vector<aiString> newTextures);
public:
	~Mesh();
};

}
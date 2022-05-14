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

    MaterialsObject mat_data;

    //when mesh is allocated into memory, we will save its location within the mesh, for easy deletion
    uint32_t index_mem;
    uint32_t vertex_mem;

private:
	Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices, Material material);
public:
	~Mesh();
};

}

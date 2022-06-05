/* ----------------- model.hpp -----------------
 * Handle loading and parsing of assimp model
 * data.
 * --------------------------------------------
*/
#pragma once


#include "mesh.hpp"

#include "config.hpp"

#include <thread>
#include <future>

#include <string>

namespace tuco {

class Model {
friend class GameObject;
friend class Antuco;
friend class GraphicsImpl;

private:
	Model();
public:
	~Model();

private:
	//if name is left null, then model will not be saved/loaded from file
	void processScene(aiNode* node, aiMesh** const meshes, aiMaterial** materials);
    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, aiMaterial** materials);
	std::vector<Vertex> processVertices(uint32_t numOfVertices, aiVector3D* aiVertices, aiVector3D* aiNormals, aiVector3t<ai_real>** textureCoords);
	std::vector<uint32_t> processIndices(uint32_t numOfFaces, aiFace* faces);
	Material processMaterial(uint32_t materialIndex, aiMaterial** materials);
	void add_mesh(const std::string& fileName, std::optional<std::string> name = std::nullopt);

private:
	std::vector<std::shared_ptr<Mesh>> model_meshes;
	std::string model_name;
	//push the model loading onto a different thread

	//reading and writing to file
	void write_to_file();
	bool file_exists(std::string model_name);
	void read_from_file();
    std::vector<Vertex> read_vertices(std::ifstream* file);
    std::vector<uint32_t> read_indices(std::ifstream* file);
    MaterialsObject read_materials(std::ifstream* file);
	std::string get_texture(std::ifstream* file);
};

}

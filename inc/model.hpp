/* ----------------- model.hpp -----------------
 * Handle loading and parsing of assimp model
 * data.
 * --------------------------------------------
*/
#pragma once


#include "mesh.hpp"

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
	void processScene(aiNode* node, aiMesh** const meshes, aiMaterial** materials);
	Mesh* processMesh(aiMesh* mesh, aiMaterial** materials);
	std::vector<Vertex> processVertices(uint32_t numOfVertices, aiVector3D* aiVertices, aiVector3D* aiNormals, aiVector3t<ai_real>** textureCoords);
	std::vector<uint32_t> processIndices(uint32_t numOfFaces, aiFace* faces);
	std::vector<aiString> processTextures(uint32_t materialIndex, aiMaterial** materials);
	void add_mesh(const std::string& fileName);

private:
	std::vector<Mesh*> model_meshes;
	//push the model loading onto a different thread
};

}

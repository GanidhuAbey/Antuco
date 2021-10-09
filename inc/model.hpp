#pragma once

//      the current game object inherits the model data, but i don't think this inheritance method plays to the best effects of what
//      we need. When u think about in inheritance hierachy would model an "is a" structure, and a gameobject is certainly not a 
//      model so the whole this falls apart.

//TODO: Revise the system so that each game object is created with a "component" model that is empty. the user will call the "add mesh()"
//      command on the game object which will call the Model class and fill the model component of the game object with model data.
//      This method would allow us to then access the required model resources from antuco and is far more inline with the
//      component oriented system from unity that our gameobjects were originally mimicking.

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
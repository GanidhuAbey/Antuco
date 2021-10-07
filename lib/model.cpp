//the implementation of the Model class
#include "model.hpp"

#include <stdexcept>

using namespace tuco;

void Model::add_mesh(const std::string& fileName) {
	//have assimp read file
	Assimp::Importer importer;
	
	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType);

	if (!scene) {
		printf("[ERROR] - Mode::add_mesh() : given model data could not be read");
		throw std::runtime_error("");
	}

	aiNode* rootNode = scene->mRootNode;
	aiMesh** const meshes = scene->mMeshes;
	aiMaterial** materials = scene->mMaterials;


	processScene(rootNode, meshes, materials);
}

Model::Model() {}
Model::~Model() {}

void Model::processScene(aiNode* node, aiMesh** const meshes, aiMaterial** materials) {
	for (uint32_t i = 0; i < node->mNumMeshes; i++) {
		Mesh mesh = processMesh(meshes[node->mMeshes[i]], materials);

		model_meshes.push_back(mesh);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processScene(node->mChildren[i], meshes, materials);
	}
}

Mesh Model::processMesh(aiMesh* mesh, aiMaterial** materials) {
	//multithread this functionality
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<aiString> texturePaths;

	auto future_vertices = std::async(std::launch::async, [&] { return processVertices(mesh->mNumVertices, mesh->mVertices, mesh->mNormals, mesh->mTextureCoords); });
	auto future_indices = std::async(std::launch::async, [&] { return processIndices(mesh->mNumFaces, mesh->mFaces); });
	auto future_texturePaths = std::async(std::launch::async, [&] { return processTextures(mesh->mMaterialIndex, materials); });

	vertices = future_vertices.get();
	indices = future_indices.get();
	texturePaths = future_texturePaths.get();
	Mesh new_mesh(vertices, indices, texturePaths);

	return new_mesh;
}

std::vector<Vertex> Model::processVertices(uint32_t numOfVertices, aiVector3D* aiVertices, aiVector3D* aiNormals, aiVector3t<ai_real>** textureCoords) {
	std::vector<Vertex> meshVertices;
	for (uint32_t i = 0; i < numOfVertices; i++) {
		aiVector3D position = aiVertices[i];
		aiVector3D normal = aiNormals[i];


		//grab texture data?
		glm::vec2 texCoord;
		if (textureCoords[0]) {
			texCoord.x = textureCoords[0][i].x;
			texCoord.y = textureCoords[0][i].y;
		}
		else texCoord = glm::vec2(0.0, 0.0);

		Vertex vertex = {
			glm::vec4(position.x, position.y, position.z, 0.0),
			glm::vec4(normal.x, normal.y, normal.z, 0.0),
			texCoord,
		};

		meshVertices.push_back(vertex);
	}

	return meshVertices;
}

std::vector<uint32_t> Model::processIndices(uint32_t numOfFaces, aiFace* faces) {
	std::vector<uint32_t> meshIndices;
	for (uint32_t i = 0; i < numOfFaces; i++) {
		if (faces->mNumIndices == 3) {
			meshIndices.push_back(faces->mIndices[0]);
			meshIndices.push_back(faces->mIndices[1]);
			meshIndices.push_back(faces->mIndices[2]);
		}
	}

	return meshIndices;
}

std::vector<aiString> Model::processTextures(uint32_t materialIndex, aiMaterial** materials) {
	aiMaterial* mat = materials[materialIndex];
	unsigned int texCount = mat->GetTextureCount(aiTextureType_DIFFUSE);
	aiString texturePath;
	char imagePath;
	std::vector<aiString> texturePaths;
	for (unsigned int i = 0; i < texCount; i++) {
		mat->GetTexture(aiTextureType_DIFFUSE, i, &texturePath);
		texturePaths.push_back(texturePath);
		//printf("the image path: %s \n", &imagePath);
	}

	return texturePaths;
}
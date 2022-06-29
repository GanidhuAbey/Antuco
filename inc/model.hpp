/* ----------------- model.hpp -----------------
 * Handle loading and parsing of assimp model
 * data.
 * --------------------------------------------
*/
#pragma once

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "mesh.hpp"

#include "config.hpp"
#include "tiny_gltf.h"

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
	
    std::vector<Vertex> processVertices(
            uint32_t numOfVertices, 
            aiVector3D* aiVertices, 
            aiVector3D* aiNormals, 
            aiVector3t<ai_real>** textureCoords);

	std::vector<uint32_t> processIndices(uint32_t numOfFaces, aiFace* faces);
	Material processMaterial(uint32_t materialIndex, aiMaterial** materials);
	void add_mesh(const std::string& fileName, std::optional<std::string> name = std::nullopt);
    bool check_gltf(const std::string& filepath);
    void add_gltf_model(const std::string& filepath);

    void process_gltf_vertices(
            tinygltf::Model model,
            const tinygltf::Primitive& primitive,
            std::vector<Vertex>& vertices);

    void assign_mesh(uint32_t index_start,
            uint32_t index_count,
            std::vector<Material> materials,
            std::vector<ImageBuffer> images);

	void process_gltf_indices(
            tinygltf::Model model,
            const tinygltf::Primitive& primitive,
            uint32_t& index_count,
            std::vector<uint32_t>& indices,
            uint32_t& index_start,
            uint32_t& vertex_start);

	void process_gltf_materials(tinygltf::Model model,
            std::vector<Material>& materials);

    void process_gltf_nodes(
            tinygltf::Node node, 
            tinygltf::Model model);

    void process_gltf_textures(tinygltf::Model model,
            std::vector<ImageBuffer>& images);

    Primitive process_assimp_primitive(aiMesh* mesh, aiMaterial** materials);

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

private:
    std::vector<Vertex> model_vertices;
    std::vector<uint32_t> model_indices;
    std::vector<Material> model_materials;
    std::vector<ImageBuffer> model_images;
    std::vector<Primitive> primitives;
};

}

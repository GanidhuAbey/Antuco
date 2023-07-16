/* ----------------- model.hpp -----------------
 * Handle loading and parsing of assimp model
 * data.
 * --------------------------------------------
*/
#pragma once

#include "mesh.hpp"

#include "config.hpp"
#include <tiny_gltf.h>
#include "node.hpp"

#include "memory_allocator.hpp"

#include <thread>
#include <future>

#include <string>

namespace tuco {

class Model {
friend class GameObject;
friend class Antuco;
friend class GraphicsImpl;

public:
	Model() = default;
	~Model() = default;

public:
    std::vector<Primitive>& get_primitives() {
        return primitives;
    }
	//if name is left null, then model will not be saved/loaded from file
	void add_mesh(const std::string& fileName, std::optional<std::string> name = std::nullopt);

private:
    bool check_gltf(const std::string& filepath);
    void add_gltf_model(const std::string& filepath);

    void process_gltf_vertices(
            tinygltf::Model model,
            const tinygltf::Primitive& primitive,
            std::vector<Vertex>& vertices);

    void assign_mesh(uint32_t index_start,
            uint32_t index_count,
            std::vector<Material> materials,
            std::vector<mem::Image> images);

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
        tinygltf::Node& node,
        tinygltf::Model& model,
        model::Node* parent_node = nullptr);

    void process_gltf_textures(tinygltf::Model model,
            std::vector<ImageBuffer>& images);

private:
	std::string model_name;
	//push the model loading onto a different thread

	//reading and writing to file
	void write_to_file();
	bool file_exists(std::string model_name);
	void read_from_file();
    std::vector<Vertex> read_vertices(std::ifstream* file);
    std::vector<uint32_t> read_indices(std::ifstream* file);
    std::vector<Material> read_materials(std::ifstream* file);
    std::vector<ImageBuffer> read_images(std::ifstream* file);
	std::string get_texture(std::ifstream* file);
    std::vector<uint32_t> linearlize_primitive(Primitive primitive);

private:
    std::vector<Vertex> model_vertices;
    std::vector<uint32_t> model_indices;
    std::vector<Material> model_materials;
    std::vector<ImageBuffer> model_images;
    std::vector<Primitive> primitives;
    std::vector<model::Node> nodes;
    std::vector<glm::mat4> transforms;
};

}

//the implementation of the Model class
#include "model.hpp"

#include "logger/interface.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <limits>

using namespace tuco;

void Model::add_mesh(const std::string& fileName, std::optional<std::string> name) {
	if (name.has_value() && file_exists(name.value())) {
		model_name = name.value();
		read_from_file();
		return;
	}

	//have assimp read file
	Assimp::Importer importer;

	auto t1 = std::chrono::high_resolution_clock::now();
	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType);
	auto t2 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> final_count = t2 - t1;
	
	if (!scene) {
		printf("[ERROR] - Mode::add_mesh() : given model data could not be read");
		throw std::runtime_error("");
	}

	aiNode* rootNode = scene->mRootNode;
	aiMesh** const meshes = scene->mMeshes;
	aiMaterial** materials = scene->mMaterials;


	processScene(rootNode, meshes, materials);

	if (name.has_value()) {
		model_name = name.value();
		write_to_file();
	}
}

Model::Model() {}
Model::~Model() {}

void Model::processScene(aiNode* node, aiMesh** const meshes, aiMaterial** materials) {
	for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        std::shared_ptr<Mesh> mesh = processMesh(meshes[node->mMeshes[i]], materials);
		
		if (mesh->is_transparent()) {
			model_meshes.push_back(mesh);
		}
		else {
			auto it = model_meshes.begin();
			model_meshes.insert(it, mesh);
		}
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processScene(node->mChildren[i], meshes, materials);
	}
}

void Model::read_from_file() {
	std::string file_name = model_name + ".bin";
	std::ifstream file(file_name, std::ios::in | std::ios::binary);
	if (!file) {
		LOG("doesn't open");
        return;
	}
   
    uint32_t read_state = 0;
    uint32_t index;
    glm::vec3 texture_opacity;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
 
    while (file.peek() != EOF) {
        //read current byte as float
        //float f;
        //file.read(reinterpret_cast<char*>(&f), sizeof(float));

        std::vector<Vertex> vertices = read_vertices(&file);
        std::vector<uint32_t> indices = read_indices(&file);
        MaterialsObject materials =  read_materials(&file);

        std::string texture_path;
        if (materials.texture_opacity.x == 1) {
            texture_path = get_texture(&file);
        }

        Mesh new_mesh(vertices, indices, materials, texture_path);

        model_meshes.push_back(std::make_unique<Mesh>(new_mesh));
    }

    file.close();
}

std::string Model::get_texture(std::ifstream* file) {
    std::string texture_path;
    while (file->peek() != EOF) {
        char c;
        file->read(&c, 1);

        texture_path.push_back(c);
    }

    return texture_path;
}

std::vector<uint32_t> Model::read_indices(std::ifstream* file) {
    bool stop = false;
    uint32_t i;
    std::vector<uint32_t> indices;
    while (!stop && file->peek() != EOF) {
        file->read(reinterpret_cast<char*>(&i), sizeof(uint32_t));

        if (i == std::numeric_limits<uint32_t>::max()) {
            stop = true;
            break;
        }
        indices.push_back(i);
    }

    return indices;
}

MaterialsObject Model::read_materials(std::ifstream* file) {
    size_t read_state = 0;
    glm::vec4 t;
    glm::vec4 a;
    glm::vec4 d;
    glm::vec4 s;

    MaterialsObject mat;

    float f;
    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    t.x = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    t.y = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    a.x = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    a.y = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    a.z = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    d.x = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    d.y = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    d.z = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    s.x = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    s.y = f;

    file->read(reinterpret_cast<char*>(&f), sizeof(float));
    s.z = f;

    t.w = 0;
    a.w = 0;
    d.w = 0;
    s.w = 0;
    mat.init(t, a, d, s);

    return mat;
}


std::vector<Vertex> Model::read_vertices(std::ifstream* file) {
    size_t read_state = 0;
    glm::vec4 pos;
    glm::vec4 norm;
    glm::vec2 tex;

    bool stop = false;
    std::vector<Vertex> vertexes;

    while (!stop && file->peek() != EOF) {
        float f;
        file->read(reinterpret_cast<char*>(&f), sizeof(float));

        switch (read_state) {
            case 0: if (f > 3.4e10) {
                        stop = true;
                        break;
                    }
                    pos.x = f;
                    read_state++;
                    break;

            case 1: pos.y = f;
                    read_state++;
                    break;

            case 2: pos.z = f;
                    read_state++;
                    break;

            case 3: norm.x = f;
                    read_state++;
                    break;

            case 4: norm.y = f;
                    read_state++;
                    break;

            case 5: norm.z = f;
                    read_state++;
                    break;

            case 6: tex.x = f;
                    read_state++;
                    break;

            case 7: { 
                    tex.y = f;

                    pos.w = 0;
                    norm.w = 0;
                    Vertex vertex(pos, norm, tex);
                    vertexes.push_back(vertex);
                    read_state = 0;
                    break;
                    }
        }
    }

    return vertexes;
}

bool Model::file_exists(std::string name) {
    std::fstream file;
	std::string file_name = name + ".bin";
    file.open(file_name, std::ios::in);

    if (file.is_open()) {
        LOG(("file is open - " + file_name).c_str());
    }

    return file.is_open();
}

void Model::write_to_file() {
    std::ofstream file;
	file.open(model_name + ".bin", std::ios::out | std::ios::binary);

	//file.write(model_name.c_str(), sizeof(model_name.c_str())); 
    uint32_t c = std::numeric_limits<uint32_t>::max();
    float f = std::numeric_limits<float>::max();
	for (size_t i = 0; i < model_meshes.size(); i++) {
		//save vertices
		for (size_t j = 0; j < model_meshes[i]->vertices.size(); j++) {
			//std::string vertex = model_meshes[i]->vertices[j].to_string();
			//file.write(vertex.c_str(), sizeof(vertex));
            //
            std::vector<float> floats = model_meshes[i]->vertices[j].linearlize();
            for (float& f : floats) {
                file.write(reinterpret_cast<char*>(&f), sizeof(float));
            }
            //Vertex vertex = model_meshes[i]->vertices[j];
            //file.write(reinterpret_cast<char*>(&vertex), sizeof(Vertex));
		} 
        file.write(reinterpret_cast<char*>(&f), sizeof(float));
		//saive indices
        for (size_t j = 0; j < model_meshes[i]->indices.size(); j+=3) {
            for (size_t k = 0; k < 3; k++) {
                uint32_t index = model_meshes[i]->indices[j + k];
                file.write(reinterpret_cast<char*>(&index), sizeof(uint32_t)); 
            } 
        }
        file.write(reinterpret_cast<char*>(&c), sizeof(uint32_t));
        
		//save materials
        std::vector<float> floats = model_meshes[i]->mat_data.linearlize();
        for (float& m : floats) {
            file.write(reinterpret_cast<char*>(&m), sizeof(float));
        }

        //save textures
        if (model_meshes[i]->mat_data.texture_opacity.x == 1) {
            std::string texture_path = std::string(model_meshes[i]->textures[0].data());
            file.write(texture_path.c_str(), texture_path.size());
        }
	}
	file.close();
}

std::shared_ptr<Mesh> Model::processMesh(aiMesh* mesh, aiMaterial** materials) {
	//multithread this functionality
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
    Material material;

	auto future_vertices = std::async(std::launch::async, [&] { return processVertices(mesh->mNumVertices, mesh->mVertices, mesh->mNormals, mesh->mTextureCoords); });
	auto future_indices = std::async(std::launch::async, [&] { return processIndices(mesh->mNumFaces, mesh->mFaces); });
	auto future_material = std::async(std::launch::async, [&] { return processMaterial(mesh->mMaterialIndex, materials); });

	vertices = future_vertices.get();
	indices = future_indices.get();
    material = future_material.get();

	Mesh new_mesh(vertices, indices, material);
	
	return std::make_shared<Mesh>(new_mesh);
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

		Vertex vertex(
			glm::vec4(position.x, position.y, position.z, 0.0), 
			glm::vec4(normal.x, normal.y, normal.z, 0.0), 
			texCoord
		);

		meshVertices.push_back(vertex);
	}

	return meshVertices;
}

std::vector<uint32_t> Model::processIndices(uint32_t numOfFaces, aiFace* faces) {
	std::vector<uint32_t> meshIndices;
	for (uint32_t i = 0; i < numOfFaces; i++) {
		if (faces[i].mNumIndices == 3) {
			meshIndices.push_back(faces[i].mIndices[0]);
			meshIndices.push_back(faces[i].mIndices[1]);
			meshIndices.push_back(faces[i].mIndices[2]);
		}
	}

	return meshIndices;
}

Material Model::processMaterial(uint32_t materialIndex, aiMaterial** materials) {
	aiMaterial* mat = materials[materialIndex];
	unsigned int texCount = mat->GetTextureCount(aiTextureType_DIFFUSE);
	aiString texturePath;
	char imagePath;
    
    Material material{};
    
    if (texCount > 0) {
		mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
        material.texturePath = texturePath;
    }

    //ambient
    aiColor3D ambient (0.f, 0.f, 0.f);
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_AMBIENT, ambient)) {
        printf("[ERROR] - could not load materials ambient colour \n");
    }

    //diffuse
    aiColor3D diffuse (0.f, 0.f, 0.f);
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
        printf("[ERROR] - could not load materials diffuse colour \n");
    }

    //specular
    aiColor3D specular (0.f, 0.f, 0.f);
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_SPECULAR, specular)) {
        printf("[ERROR] - could not load materials specular colour \n");
    } 

    //opacity
    float opacity = 0.0f;
    if (AI_SUCCESS != mat->Get(AI_MATKEY_OPACITY, opacity)) {
        LOG("[ERROR] - could not load materials opacity data");        
    }

    float roughness = 0.0f;
    if (AI_SUCCESS !=mat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughness)) {
        LOG("[ERROR] - could not load material roughness");
    }
 

    material.ambient = ambient;
    material.diffuse = diffuse;
    material.specular = specular;

    material.opacity = opacity;

    return material;

}

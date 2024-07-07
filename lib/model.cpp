// the implementation of the Model class
#include "model.hpp"

#define GLM_FORCE_QUAT_DATA_XYZW
#include "config.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "logger/interface.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace tuco;

bool Model::check_gltf(const std::string &filepath) {
  std::string ext = get_extension_from_file_path(filepath);

  if (ext == "glb" || ext == "gltf") {
    return true;
  }
  return false;
}

void Model::add_gltf_model(const std::string &filepath) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string warn;
  std::string err;

  std::string ext = get_extension_from_file_path(filepath);

  bool ret;
  if (ext == "gltf")
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
  else
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);

  // print any warnings/errors
  if (!warn.empty()) {
    WARN(warn.c_str());
  }
  if (!err.empty()) {
    ERR(err.c_str());
  }
  if (!ret) {
    ERR("something went wrong while trying to import GLTF model");
  }

  // get scene
  const tinygltf::Scene &scene = model.scenes[0]; // use model.defaultScene?

  // process materials
  // process_gltf_materials(model, model_materials);

  // process images
  process_gltf_textures(model, model_images);

  // process nodes
  for (size_t i = 0; i < scene.nodes.size(); i++) {
    process_gltf_nodes(model.nodes[scene.nodes[i]], model);
  }
}

void Model::process_gltf_nodes(tinygltf::Node &node, tinygltf::Model &model,
                               model::Node *parent_node) {

  auto unique_node = std::make_unique<model::Node>();
  unique_node->add_parent(parent_node);

  // uint32_t index_point = model_indices.size();
  // uint32_t vertex_point = model_vertices.size();
  auto index_point = uint32_t(0);
  auto vertex_point = uint32_t(0);
  uint32_t index_count = 0;

  auto matrix = glm::mat4(1.0f);

  // process translations for nodes
  if (node.translation.size() == 3) {
    matrix = glm::translate(matrix,
                            glm::vec3(glm::make_vec3(node.translation.data())));
  }
  if (node.rotation.size() == 4) {
    // auto v = ;
    glm::quat q = glm::make_quat(node.rotation.data());
    matrix *= glm::mat4(q);
  }
  if (node.scale.size() == 3) {
    matrix = glm::scale(matrix, glm::vec3(glm::make_vec3(node.scale.data())));
  }
  if (node.matrix.size() == 16) {
    matrix = glm::make_mat4x4(node.matrix.data());
  }

  unique_node->add_matrix(matrix);

  // each node may have children
  for (auto &children : node.children) {
    process_gltf_nodes(model.nodes[children], model, unique_node.get());
  }

  if (node.mesh > -1) {
    const tinygltf::Mesh mesh = model.meshes[node.mesh];
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
      const tinygltf::Primitive &primitive = mesh.primitives[i];
      index_point = static_cast<uint32_t>(model_indices.size());
      vertex_point = static_cast<uint32_t>(model_vertices.size());
      // process vertices
      process_gltf_vertices(model, primitive, model_vertices);

      // process indices
      index_count = 0;
      process_gltf_indices(model, primitive, index_count, model_indices,
                           index_count, // can delete this from function
                           vertex_point);

      // assign material to mesh
      Primitive prim{};
      prim.index_start = index_point;
      prim.index_count = index_count;
      prim.mat_index = primitive.material;
      if (prim.mat_index == -1) {
        // prim.image_index = -1;
        //  create new material
        // auto mat = Material{};
        // mat.ambient = glm::vec4(1.0);
        // mat.diffuse = glm::vec4(1.0);
        // mat.opacity = 1.0f;
        // mat.specular = glm::vec4(0.0f);

        // prim.mat_index = model_materials.size();
        // model_materials.push_back(mat);
      } else {
        prim.image_index = -1; // model_materials[prim.mat_index].image_index;
      }
      prim.transform_index = transforms.size();
      primitives.push_back(prim);
    }
  }

  // converge current transform with nodes parent transforms.
  model::Node *prev_node = parent_node;
  glm::mat4 final_matrix = matrix;
  while (prev_node) {
    final_matrix = prev_node->get_transform() * final_matrix;
    prev_node = prev_node->get_parent();
  }

  transforms.push_back(final_matrix);

  if (parent_node) {
    parent_node->add_node(std::move(unique_node));
  } else {
    nodes.push_back(std::move(unique_node));
  }
}

// NOTE: assumes model.images.size() == model.textures.size();
void Model::process_gltf_textures(tinygltf::Model model,
                                  std::vector<ImageBuffer> &images) {
  images.resize(model.images.size());

  for (size_t i = 0; i < images.size(); i++) {
    tinygltf::Image &image = model.images[i];
    uint32_t index = model.textures[i].source;
    if (image.component == 3) {
      msg::print_line("probably doesnt work");
      images[index].buffer_size = image.width * image.height * 4;
      unsigned char *rgba = new unsigned char[images[index].buffer_size];
      unsigned char *rgb = &image.image[0];
      for (size_t i = 0; i < image.width * image.height; i++) {
        memcpy(rgba, rgb, sizeof(unsigned char) * 3);
        rgba += 4;
        rgb += 3;
      }

      images[index].buffer.resize(1);
      images[index].buffer[0] = *std::move(rgba);

      // delete[] rgba;
    } else {
      // images[index].buffer.resize(1);
      images[index].buffer = image.image;
      images[index].buffer_size = image.image.size();
    }

    images[index].width = image.width;
    images[index].height = image.height;
  }
}

void Model::process_gltf_materials(tinygltf::Model model,
                                   std::vector<Material> &materials) {
  /*
  materials.resize(model.materials.size());
  for (size_t i = 0; i < materials.size(); i++) {
      tinygltf::Material mat = model.materials[i];

      //load diffuse
      float alpha = 1.0f;
      glm::vec4 diffuse = glm::vec4(1.0, 1.0, 1.0, 1.0);
      if (mat.values.find("baseColorFactor") != mat.values.end()) {
                      diffuse =
  glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
              }
      materials[i].diffuse = diffuse;

      materials[i].opacity = diffuse.w;
      if (mat.alphaMode == "OPAQUE") {
          materials[i].opacity = 1.0f;
      }


      materials[i].ambient = glm::vec4(1.0, 1.0, 1.0, 1.0); //not required
  anymore

      //load roughness
      if (mat.values.find("roughnessFactor") != mat.values.end()) {
          materials[i].specular.r = mat.values["roughnessFactor"].Factor();
      }

      //load metallic factor
      if (mat.values.find("metallicFactor") != mat.values.end()) {
          materials[i].specular.g = mat.values["metallicFactor"].Factor();
      }

      //get texture data
      if (mat.values.find("baseColorTexture") != mat.values.end()) {
          materials[i].image_index =
  mat.values["baseColorTexture"].TextureIndex();
      }
      else {
          materials[i].image_index = -1;
      }
  }
  */
}

void Model::process_gltf_indices(tinygltf::Model model,
                                 const tinygltf::Primitive &primitive,
                                 uint32_t &index_count,
                                 std::vector<uint32_t> &indices,
                                 uint32_t &index_start,
                                 uint32_t &vertex_start) {
  const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
  const tinygltf::BufferView &buffer_view =
      model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer &buffer = model.buffers[buffer_view.buffer];

  index_count += static_cast<uint32_t>(accessor.count);

  switch (accessor.componentType) {
  case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
    const uint32_t *buf = reinterpret_cast<const uint32_t *>(
        &buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    for (size_t index = 0; index < accessor.count; index++) {
      indices.push_back(buf[index] + vertex_start);
    }
    break;
  }
  case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
    const uint16_t *buf = reinterpret_cast<const uint16_t *>(
        &buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    for (size_t index = 0; index < accessor.count; index++) {
      indices.push_back(static_cast<uint32_t>(buf[index] + vertex_start));
    }
    break;
  }
  case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
    const uint8_t *buf = reinterpret_cast<const uint8_t *>(
        &buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    for (size_t index = 0; index < accessor.count; index++) {
      indices.push_back(static_cast<uint32_t>(buf[index] + vertex_start));
    }
    break;
  }
  default:
    std::cerr << "Index component type " << accessor.componentType
              << " not supported!" << std::endl;
    break;
  }
}

void Model::process_gltf_vertices(tinygltf::Model model,
                                  const tinygltf::Primitive &primitive,
                                  std::vector<Vertex> &vertices) {
  const float *position_buffer = nullptr;
  const float *normal_buffer = nullptr;
  const float *tex_coord_buffer = nullptr;
  size_t vertex_count = 0;

  if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
    const tinygltf::Accessor &accessor =
        model.accessors[primitive.attributes.find("POSITION")->second];
    const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
    position_buffer = reinterpret_cast<const float *>(
        &(model.buffers[view.buffer]
              .data[accessor.byteOffset + view.byteOffset]));
    vertex_count = accessor.count;
  }

  if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
    const tinygltf::Accessor &accessor =
        model.accessors[primitive.attributes.find("NORMAL")->second];
    const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
    normal_buffer = reinterpret_cast<const float *>(
        &(model.buffers[view.buffer]
              .data[accessor.byteOffset + view.byteOffset]));
  }

  if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
    const tinygltf::Accessor &accessor =
        model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
    const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
    tex_coord_buffer = reinterpret_cast<const float *>(
        &(model.buffers[view.buffer]
              .data[accessor.byteOffset + view.byteOffset]));
  }

  for (size_t v = 0; v < vertex_count; v++) {
    glm::vec4 pos = glm::vec4(glm::make_vec3(&position_buffer[v * 3]), 1.0f);
    glm::vec4 norm = glm::vec4(
        glm::vec3(normal_buffer ? glm::make_vec3(&normal_buffer[v * 3])
                                : glm::vec3(0.0f)),
        1.0f);

    glm::vec2 uv = tex_coord_buffer ? glm::make_vec2(&tex_coord_buffer[v * 2])
                                    : glm::vec3(0.0f);

    Vertex vertex(pos, norm, uv);

    vertices.push_back(vertex);
  }
}

void Model::add_mesh(const std::string &fileName,
                     std::optional<std::string> name) {
  // NOTE: disabled reading from files
  /*
      if (name.has_value() && file_exists(name.value())) {
              model_name = name.value();
              //read_from_file();
              return;
      }
  */

  if (check_gltf(fileName)) {
    add_gltf_model(fileName);
    return;
  } else {
    ERR("could not add gltf model");
    return;
  }
}

Model::Model() {}
Model::~Model() {}

void Model::read_from_file() {
  std::string file_name = model_name + ".bin";
  std::ifstream file(file_name, std::ios::in | std::ios::binary);
  if (!file) {
    ERR("file: [%s] could not be opened.", file_name);
    return;
  }

  uint32_t read_state = 0;
  uint32_t index;
  glm::vec3 texture_opacity;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;

  while (file.peek() != EOF) {
    // read current byte as float
    // float f;
    // file.read(reinterpret_cast<char*>(&f), sizeof(float));

    std::vector<Vertex> vertices = read_vertices(&file);
    std::vector<uint32_t> indices = read_indices(&file);
    std::vector<ImageBuffer> images = read_images(&file);
    std::vector<Material> materials = read_materials(&file);

    model_vertices = vertices;
    model_indices = indices;
    // model_materials = materials;
  }

  file.close();
}

std::vector<ImageBuffer> Model::read_images(std::ifstream *file) {
  std::vector<ImageBuffer> images;

  while (file->peek() != EOF) {
    ImageBuffer ib;
    // get buffer size
    uint32_t buffer_size;
    file->read(reinterpret_cast<char *>(&ib.buffer_size), sizeof(uint32_t));

    // if at end of image data, then buffer size will be uint32_t::max
    if (ib.buffer_size == UINT32_MAX)
      break;

    // get image data
    file->read(reinterpret_cast<char *>(&ib.buffer), sizeof(ib.buffer_size));

    // get image dimensions
    file->read(reinterpret_cast<char *>(&ib.width), sizeof(uint32_t));
    file->read(reinterpret_cast<char *>(&ib.height), sizeof(uint32_t));

    images.push_back(ib);
  }
  return images;
}

std::string Model::get_texture(std::ifstream *file) {
  std::string texture_path;
  while (file->peek() != EOF) {
    char c;
    file->read(&c, 1);

    texture_path.push_back(c);
  }

  return texture_path;
}

std::vector<uint32_t> Model::read_indices(std::ifstream *file) {
  bool stop = false;
  uint32_t i;
  std::vector<uint32_t> indices;
  while (!stop && file->peek() != EOF) {
    file->read(reinterpret_cast<char *>(&i), sizeof(uint32_t));

    if (i == std::numeric_limits<uint32_t>::max()) {
      stop = true;
      break;
    }
    indices.push_back(i);
  }

  return indices;
}

std::vector<Material> Model::read_materials(std::ifstream *file) {
  size_t read_state = 0;
  glm::vec4 t;
  glm::vec4 ambience;
  glm::vec4 diffuse;
  glm::vec4 specular;

  std::vector<Material> mats;

  while (file->peek() != EOF) {
    float f;
    // read ambience
    for (size_t i = 0; i < 3; i++) {
      file->read(reinterpret_cast<char *>(&f), sizeof(float));
      ambience[i] = f;
    }

    // read diffuse
    for (size_t i = 0; i < 3; i++) {
      file->read(reinterpret_cast<char *>(&f), sizeof(float));
      diffuse[i] = f;
    }

    // read specular
    for (size_t i = 0; i < 3; i++) {
      file->read(reinterpret_cast<char *>(&f), sizeof(float));
      specular[i] = f;
    }
  }

  return mats;
}

std::vector<Vertex> Model::read_vertices(std::ifstream *file) {
  size_t read_state = 0;
  glm::vec4 pos;
  glm::vec4 norm;
  glm::vec2 tex;

  bool stop = false;
  std::vector<Vertex> vertexes;

  while (!stop && file->peek() != EOF) {
    float f;
    file->read(reinterpret_cast<char *>(&f), sizeof(float));

    switch (read_state) {
    case 0:
      if (f > 3.4e10) {
        stop = true;
        break;
      }
      pos.x = f;
      read_state++;
      break;

    case 1:
      pos.y = f;
      read_state++;
      break;

    case 2:
      pos.z = f;
      read_state++;
      break;

    case 3:
      norm.x = f;
      read_state++;
      break;

    case 4:
      norm.y = f;
      read_state++;
      break;

    case 5:
      norm.z = f;
      read_state++;
      break;

    case 6:
      tex.x = f;
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
    INFO("file [%s] exists.", file_name.c_str());
  }

  return file.is_open();
}

std::vector<uint32_t> Model::linearlize_primitive(Primitive primitive) {
  std::vector<uint32_t> lin;
  lin.push_back(primitive.image_index);
  lin.push_back(primitive.index_count);
  lin.push_back(primitive.index_start);
  lin.push_back(primitive.mat_index);
  lin.push_back(static_cast<uint32_t>(primitive.is_transparent));

  return lin;
}

void Model::write_to_file() {
  std::ofstream file;
  file.open(model_name + ".bin", std::ios::out | std::ios::binary);

  // file.write(model_name.c_str(), sizeof(model_name.c_str()));
  uint32_t c = std::numeric_limits<uint32_t>::max();
  float f = std::numeric_limits<float>::max();

  // save vertices
  for (Vertex &vertex : model_vertices) {
    std::vector<float> vertex_linearlized = vertex.linearlize();
    for (float &num : vertex_linearlized) {
      file.write(reinterpret_cast<char *>(&num), sizeof(float));
    }
  }
  file.write(reinterpret_cast<char *>(&f), sizeof(float));

  // save indices
  for (uint32_t &index : model_indices) {
    file.write(reinterpret_cast<char *>(&index), sizeof(uint32_t));
  }
  file.write(reinterpret_cast<char *>(&c), sizeof(uint32_t));

  // save image data
  // TODO: load image with stb_image.h and save image buffer data to file
  // (faster at runtime) and consistent with gltf importing

  // save primitives
  for (Primitive &prim : primitives) {
    std::vector<uint32_t> lin = linearlize_primitive(prim);
    for (uint32_t &u_int : lin) {
      file.write(reinterpret_cast<char *>(&lin), sizeof(uint32_t));
    }
  }

  // save images
  for (size_t i = 0; i < model_images.size(); i++) {
    unsigned char image_buffer = model_images[i].buffer[0];
    uint32_t buffer_size = model_images[i].buffer_size;

    uint32_t image_width = model_images[i].width;
    uint32_t image_height = model_images[i].height;

    file.write(reinterpret_cast<char *>(&buffer_size), sizeof(uint32_t));
    file.write(reinterpret_cast<char *>(&image_buffer), buffer_size);
    file.write(reinterpret_cast<char *>(image_width), sizeof(uint32_t));
    file.write(reinterpret_cast<char *>(image_height), sizeof(uint32_t));
  }

  file.write(reinterpret_cast<char *>(&c), sizeof(uint32_t));

  /*
  // save materials
  for (Material &mat : model_materials) {
    std::vector<float> mat_floats = mat.linearlize();
    for (float &num : mat_floats) {
      file.write(reinterpret_cast<char *>(&num), sizeof(float));
    }
    file.write(reinterpret_cast<char *>(&mat.image_index), sizeof(uint32_t));
  }
  */

  file.close();
}

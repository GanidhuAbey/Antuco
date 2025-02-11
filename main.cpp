// CREDITS: tyrant_monkey - bmw model

#include "antuco.hpp"
#include "antuco_enums.hpp"
#include <scene.hpp>

#include <chrono>
#include <iostream>

const int WIDTH = 1600;
const int HEIGHT = 900;

const float PLAYER_SPEED = 2.0f;
const double MOUSE_SENSITIVITY = 0.1f;

#define TIME_IT std::chrono::high_resolution_clock::now();

const std::string CURRENT_FILE = __FILE__;
const std::string SOURCE_PATH =
    CURRENT_FILE.substr(0, CURRENT_FILE.rfind("\\"));

/// <summary>
///  rotates point (rotation_item) with respect to a given point
///  (rotation_point) by a given angle (rotation_angle)
/// </summary>
/// <param name="rotation_item"> the point that will get rotated </param>
/// <param name="rotation_point"> the point that is being rotated around
/// </param> <param name="rotation_angle"> the amount that rotation_item will be
/// rotated by in degrees </param> <returns> new point thats rotated around a
/// point </returns>
glm::vec2 rotate_around_point(glm::vec2 rotation_item, glm::vec2 rotation_point,
                              float rotation_angle) {
  // define rotation matrix
  float theta = glm::radians(rotation_angle);
  glm::mat2 rotation_matrix = {
      glm::cos(theta),
      -glm::sin(theta),
      glm::sin(theta),
      glm::cos(theta),
  };

  // rotate rotation_item
  glm::vec2 rotation = rotation_item - rotation_point;
  glm::vec2 rotate_point = rotation_matrix * rotation_item;

  return rotate_point + rotation_point;
}

int main() {
    // when beginning a new game, one must first create the window, but user
    // should only ever interact with the library wrapper "Antuco_lib"
    tuco::Antuco &antuco = tuco::Antuco::get_engine();

    std::string root_project = get_project_root(__FILE__);

    // create window
    tuco::Window *window = antuco.init_window(WIDTH, HEIGHT, "SOME COOL TITLE");

    // create engine here so the title exists?
    antuco.init_graphics(tuco::RenderEngine::Vulkan);

    int never_used = 1;

    // in order to create a scene we need a camera, a light, and a object right?

    // create camera
    glm::vec3 camera_pos = glm::vec3(0.0, 1.0, 0.0);
    // the direction the camera is looking at
    // think of the camera_pos moving away from the center, inorder to look at the
    // center then you need to look in the negative z axis to "look back" at the
    // center.
    glm::vec3 camera_face = glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 camera_orientation = glm::vec3(0.0, -1.0, 0.0);

    tuco::Camera *main_camera =
        antuco.create_camera(camera_pos, camera_face, camera_orientation,
                            glm::radians(45.0f), 0.01f, 150.0f);

    // create some light for the scene
    glm::vec3 light_position = glm::vec3(-3.58448f, 7.69584f, 11.7122f);
    glm::vec3 light_look_at = glm::vec3(0.0f, 0.0f, 0.0f);
    tuco::DirectionalLight &light = antuco.create_spotlight(
        light_position, light_look_at, glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(0.0, 1.0, 0.0), true);

    // create a simple game object
    // tuco::GameObject* light_mesh = antuco.create_object();

    // light_mesh->add_mesh("objects/test_object/white.obj");
    // light_mesh->translate(glm::vec3(0.0f, 4.0f, 0.0f));
    // light_mesh->scale(glm::vec3(0.1, 0.1, 0.1));

    tuco::SceneData* scene = antuco.create_scene();

    auto t1 = TIME_IT;

#if defined(__APPLE__)
    auto floor = antuco.create_object();
    floor->add_mesh(root_project + "/objects/antuco-files/mac/cube.glb");
    floor->scale(glm::vec3(1, 0.01, 1));

    floor->get_material().albedo = glm::vec3(1.0);
    floor->get_material().metallic = 0.1;
    floor->get_material().roughness = 0.8;

    auto object = antuco.create_object();
    object->add_mesh(root_project + "/objects/antuco-files/mac/texturedCube.glb");
    object->scale(glm::vec3(0.1, 0.1, 0.1));
    object->translate(glm::vec3(0, 1, 0));

    object->get_material().albedo = glm::vec3(1.0, 0.0, 0.0);
    object->get_material().metallic = 0.1;
    object->get_material().roughness = 0.9;

#elif defined(_WIN32) || defined(_WIN64)
    //auto floor = antuco.create_object();
    //floor->add_mesh(root_project + "\\objects\\antuco-files\\windows\\cube.glb");
    //floor->scale(glm::vec3(1, 0.01, 1));

    //floor->get_material().albedo = glm::vec3(0.0, 0.0, 1.0);
    //floor->get_material().metallic = 0.1;
    //floor->get_material().roughness = 0.8;

    auto gun = antuco.create_object();
    gun->add_mesh(root_project + "\\objects\\antuco-files\\windows\\Gun\\gun.glb");
    gun->scale(glm::vec3(0.1));
    gun->translate(glm::vec3(0, 1, 0));

    gun->get_material()->albedo = glm::vec3(1.0, 1.0, 1.0);
    gun->get_material()->metallic = 0.7;
    gun->get_material()->roughness = 0.3;

    gun->get_material()->setBaseColorTexture(root_project + "\\objects\\antuco-files\\windows\\Gun\\Textures\\Cerberus_A.tga");
    gun->get_material()->setRoughnessMetallicTexture(root_project + "\\objects\\antuco-files\\windows\\Gun\\Textures\\Cerberus_R.tga");
    gun->get_material()->setMetallicTexture(root_project + "\\objects\\antuco-files\\windows\\Gun\\Textures\\Cerberus_M.tga");

    //scene->set_ibl(root_project + "\\objects\\antuco-files\\textures\\environment\\royal_esplanade_4k.hdr");
    
    //antuco.add_skybox(root_project + "\\objects\\antuco-files\\textures\\environment\\royal_esplanade_skybox");
    scene->set_skybox(root_project + "\\objects\\antuco-files\\textures\\environment\\brown_photostudio_01_4k.hdr");

#endif

    // light_mesh->scale(glm::vec3(0.2));

    auto t2 = TIME_IT;

    std::chrono::duration<double, std::milli> delta_time = t2 - t1;
    double delta = delta_time.count();

    // basic game loop
    bool game_loop = true;

    float pitch = 0.0f;
    float yaw = 180.0f;

    double x_pos = WIDTH / 2;
    double y_pos = HEIGHT / 2;

    double p_xpos = x_pos;
    double p_ypos = y_pos;

    // lock cursor
    window->lock_cursor();

    float timer = 0.0f;

    // update light position
    light.update(light_position);

    while (game_loop) {
        // cut the y axis to analze the rotation from two dimensions.

        auto t1 = TIME_IT;
        timer += 0.0001f;
        // query mouse position
        double offset_x = (x_pos - p_xpos) * MOUSE_SENSITIVITY;
        double offset_y = (y_pos - p_ypos) * MOUSE_SENSITIVITY;

        yaw += offset_x;
        pitch = glm::clamp(pitch - offset_y, -89.0, 89.0);

        camera_face.x = static_cast<float>((glm::sin(glm::radians(yaw))) *
                                            glm::cos(glm::radians(pitch)));
        camera_face.y = static_cast<float>(glm::sin(glm::radians(pitch)));
        camera_face.z = static_cast<float>((glm::cos(glm::radians(yaw))) *
                                            glm::cos(glm::radians(pitch)));

        camera_face = glm::normalize(camera_face);
        glm::vec3 camera_right =
            glm::normalize(glm::cross(camera_face, camera_orientation));
        glm::vec3 camera_up = glm::normalize(glm::cross(camera_face, camera_right));

        // handle movement
        if (window->get_key_state(tuco::WindowInput::W)) {
            camera_pos += PLAYER_SPEED * camera_face * (float)delta;
        } else if (window->get_key_state(tuco::WindowInput::A)) {
            camera_pos -= camera_right * PLAYER_SPEED * static_cast<float>(delta);
        } else if (window->get_key_state(tuco::WindowInput::D)) {
            camera_pos += camera_right * PLAYER_SPEED * static_cast<float>(delta);
        } else if (window->get_key_state(tuco::WindowInput::S)) {
            camera_pos -= PLAYER_SPEED * camera_face * static_cast<float>(delta);
        } else if (window->get_key_state(tuco::WindowInput::Q)) {
            camera_pos +=
                camera_orientation * PLAYER_SPEED * 0.1f * static_cast<float>(delta);
        } else if (window->get_key_state(tuco::WindowInput::E)) {
            camera_pos -=
                camera_orientation * PLAYER_SPEED * 0.1f * static_cast<float>(delta);
        }
        if (window->get_key_state(tuco::WindowInput::X)) {
            light.update(camera_pos);
        }

        // render the objects onto the screen
        main_camera->update(camera_pos, camera_face);
        antuco.render();

        p_xpos = x_pos;
        p_ypos = y_pos;
        window->get_mouse_pos(&x_pos, &y_pos);

        // check if user has closed the window
        auto t2 = TIME_IT;
        // frame time
        delta_time = t2 - t1;
        delta = delta_time.count() * 0.001;

        game_loop = !window->check_window_status(tuco::WindowStatus::CLOSE_REQUEST);

        // close input
        if (window->get_key_state(tuco::WindowInput::F)) {
            window->close();
            game_loop = false;
        }
    }
}

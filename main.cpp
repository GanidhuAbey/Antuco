#include "antuco.hpp"

#include <iostream>
#include <chrono>

const int WIDTH = 800;
const int HEIGHT = 600;

const float PLAYER_SPEED = 0.05f;
const double MOUSE_SENSITIVITY = 0.1f;

#define TIME_IT std::chrono::high_resolution_clock::now();

int main() {
	//when beginning a new game, one must first create the window, but user should only ever interact with the library wrapper "Antuco_lib"
	tuco::Antuco& antuco = tuco::Antuco::get_engine();
	//create window
	tuco::Window* window = antuco.init_window(WIDTH, HEIGHT, "SOME COOL TITLE");

	//create engine here so the title exists?
	antuco.init_graphics();

	//in order to create a scene we need a camera, a light, and a object right?

	//create camera
	glm::vec3 camera_pos = glm::vec3(0.0, 0.0, 3.0);
	//the direction the camera is looking at
	//think of the camera_pos moving away from the center, inorder to look at the center then
	//you need to look in the negative z axis to "look back" at the center.
	glm::vec3 camera_face = glm::vec3(0.0, 0.0, -1.0);
	glm::vec3 camera_orientation = glm::vec3(0.0, -1.0, 0.0);

	tuco::Camera* main_camera = antuco.create_camera(camera_pos, camera_face, camera_orientation, glm::radians(45.0f), 0.01f, 150.0f);

	//create some light for the scene
	tuco::Light* light = antuco.create_light(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(1.0, 1.0, 1.0));

	//create a simple game object
	tuco::GameObject* another = antuco.create_object();
	tuco::GameObject* some_object = antuco.create_object();

	auto t1 = TIME_IT;	
	another->add_mesh("objects/test_object/big_texture.obj");
	another->scale(glm::vec3(0.1, 0.1, 0.1));
	
	some_object->add_mesh("objects/test_object/with_texture.obj"); //will hope texture data is located within model data.
	some_object->scale(glm::vec3(0.1, 0.1, 0.1));


	another->translate(glm::vec3(0, -0.1, 0));	
	auto t2 = TIME_IT;

	std::chrono::duration<double, std::milli> final_count = t2 - t1;
	printf("time to load model: %f \n", final_count.count());
	
	//basic game loop
	bool game_loop = true;

	float pitch = 0.0f;
	float yaw = 180.0f;

	double x_pos = WIDTH / 2;
	double y_pos = HEIGHT / 2;

	double p_xpos = x_pos;
	double p_ypos = y_pos;

	//lock cursor
	window->lock_cursor();

	while (game_loop) {
		//query mouse position
		double offset_x = (x_pos - p_xpos) * MOUSE_SENSITIVITY;
		double offset_y = (y_pos - p_ypos) * MOUSE_SENSITIVITY;

		yaw += offset_x;
		pitch = std::clamp(pitch - offset_y, -89.0, 89.0);

		camera_face.x = (float) (glm::sin(glm::radians(yaw))) * glm::cos(glm::radians(pitch));
		camera_face.y = (float) glm::sin(glm::radians(pitch));
		camera_face.z = (float) (glm::cos(glm::radians(yaw))) * glm::cos(glm::radians(pitch));

		camera_face = glm::normalize(camera_face);

		//printf("yaw: %f | pitch: %f \n", yaw, pitch);


		//take some input
		if (window->get_key_state(tuco::WindowInput::X)) {
			printf("the x button has been pressed \n");
		}
		//handle movement
		if (window->get_key_state(tuco::WindowInput::W)) {
			camera_pos += PLAYER_SPEED * camera_face;
		}	
		else if (window->get_key_state(tuco::WindowInput::A)) {
			camera_pos -= glm::normalize(glm::cross(camera_face, camera_orientation)) * PLAYER_SPEED;
		}	
		else if (window->get_key_state(tuco::WindowInput::D)) {	
			camera_pos += glm::normalize(glm::cross(camera_face, camera_orientation)) * PLAYER_SPEED;
		}
		else if (window->get_key_state(tuco::WindowInput::S)) {
			camera_pos -= PLAYER_SPEED * camera_face;
		}

		//printf("camera_pos: <%f, %f, %f> \n", camera_pos.x, camera_pos.y, camera_pos.z);

		//render the objects onto the screen
		main_camera->update(camera_pos, camera_face);
		antuco.render();

		p_xpos = x_pos;
		p_ypos = y_pos;
		window->get_mouse_pos(&x_pos, &y_pos);

		//check if user has closed the window
		game_loop = !window->check_window_status(tuco::WindowStatus::CLOSE_REQUEST);
	}
}
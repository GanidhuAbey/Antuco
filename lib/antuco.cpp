#include "antuco.hpp"

#include "logger/interface.hpp"

#include <iostream>

using namespace tuco;

//initialize the class
Antuco Antuco::Antuco_instance;

Antuco::Antuco() {
	//what should i do here?
}

Antuco::~Antuco() {
	delete p_graphics;
}

/* Window Initialization */
Window* Antuco::init_window(int w, int h, const char* title) {
	Window* window = new Window(w, h, title);

	pWindow = window;
	
	return window;
}

void Antuco::init_graphics() {
	p_graphics = new Graphics(pWindow);
}

/* World Object Initalization */
Light* Antuco::create_light(glm::vec3 light_pos, glm::vec3 light_target, glm::vec3 light_color, glm::vec3 up, bool cast_shadows) {

	Light* light = new Light(light_pos, light_target, light_color, up);
	light->light_index = lights.size();

	if (cast_shadows && shadow_casters.size() < MAX_SHADOW_CASTERS) {
		shadow_casters.push_back(lights.size());
	}
	else if (shadow_casters.size() >= MAX_SHADOW_CASTERS) {
		//log error here
		ERR_V_MSG("too many shadow casters");
	}
	lights.push_back(light);

	return light;
}

Camera* Antuco::create_camera(glm::vec3 eye, glm::vec3 facing, glm::vec3 up, float yfov, float near, float far) {
	float aspect_ratio = pWindow->get_width() / (float) pWindow->get_height();
	
	Camera* camera = new Camera(eye, facing, up, yfov, aspect_ratio, near, far);

	cameras.push_back(camera);

	return camera;
}

GameObject* Antuco::create_object() {
	GameObject* game_object = new GameObject;

	objects.push_back(game_object);

	return game_object;
}

void Antuco::render() {
	//printf("size: %zu \n", lights.size());
	
	//check and update the camera information
	//printf("camera position: <%f, %f, %f>", cameras[0]->construct_world_to_camera())
	/*
	printf("====================================================================== \n");
	for (size_t i = 0; i < 4; i++) {
		printf("| %f | %f | %f | %f | \n", cameras[0]->modelToCamera[i][0], cameras[0]->modelToCamera[i][1], cameras[0]->modelToCamera[i][2], cameras[0]->modelToCamera[i][3]);
	}
	printf("====================================================================== \n");
	*/
	p_graphics->update_camera(cameras[0]->modelToCamera, cameras[0]->cameraToScreen);
	//check and update the light information
	//the engine can't handle multiple light sources right now, so we will only use the first light created
	p_graphics->update_light(lights, shadow_casters);

	//check and update the game object information, this would be where we update the command buffers as neccesary
	p_graphics->update_draw(objects);

	//send command to draw
}

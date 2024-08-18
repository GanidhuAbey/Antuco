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
Window* Antuco::init_window(int w, int h, const char* title) 
{
	Window* window = new Window(w, h, title);

	pWindow = window;
	
	return window;
}

void Antuco::init_graphics(RenderEngine api) 
{
	//when/if other render api's implemented, add graphics interface, to which
	//each render api object would obey.
	Antuco::api = api;
	p_graphics = new Graphics(pWindow); 
}

GraphicsImpl* Antuco::get_backend()
{
	return p_graphics->p_graphics.get();
}

/* World Object Initalization */
DirectionalLight& Antuco::create_spotlight(glm::vec3 light_pos, glm::vec3 light_target, glm::vec3 light_color, glm::vec3 up, bool cast_shadows) {

	DirectionalLight light(light_pos, light_target, light_color, up);
	light.light_index = directional_lights.size();

	if (cast_shadows && shadow_casters.size() < MAX_SHADOW_CASTERS) {
		shadow_casters.push_back(light.light_index);
	}
	else if (shadow_casters.size() >= MAX_SHADOW_CASTERS) {
		//log error here
		ERR("Too many shadow casters");
	}
	directional_lights.push_back(light);

	return directional_lights[directional_lights.size() - 1];
}

PointLight& Antuco::create_point_light(glm::vec3 pos, glm::vec3 colour) {
    PointLight light(pos, colour);

    point_lights.push_back(light);

    return point_lights[point_lights.size() - 1];
}

Camera* Antuco::create_camera(glm::vec3 eye, glm::vec3 facing, glm::vec3 up, float yfov, float n, float f) {
	float aspect_ratio = pWindow->get_width() / (float) pWindow->get_height();
	
	Camera* camera = new Camera(eye, facing, up, yfov, aspect_ratio, n, f);

	cameras.push_back(camera);

	return camera;
}

GameObject* Antuco::create_object() {
	objects.push_back(std::make_unique<GameObject>());
	return objects[objects.size() - 1].get();
}

SceneData* Antuco::create_scene()
{
	scene = std::make_unique<SceneData>();
	return scene.get();
}


void Antuco::render() {	
	//check and update the camera information
	p_graphics->update_camera(cameras[0]->modelToCamera, cameras[0]->cameraToScreen, glm::vec4(cameras[0]->pos, 0));
	//check and update the light information
	//the engine can't handle multiple light sources right now, so we will only use the first light created
	p_graphics->update_light(directional_lights, shadow_casters);

	//check and update the game object information, this would be where we update the command buffers as neccesary
	p_graphics->update_draw(objects);
}

/* LIGHT WRAPPER OVER THE MAIN FUNCTIONALITY OF THE ENGINE FOR THE USER TO INTERACT WITH */
#pragma once

#include "antuco_enums.hpp"
#include "window.hpp"
#include "world_objects.hpp"
#include "graphics.hpp"

#include <vector>

namespace tuco {

class Antuco {
	/* Window */
public:
	Window* init_window(int w, int h, const char* title);
	void init_graphics();
private:
	Window* pWindow;
	Graphics* p_graphics;

	/* World Objects */
public:
	Light* create_light(glm::vec3 light_pos, glm::vec3 light_color);
	Camera* create_camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float yfov, float near, float far);
	GameObject* create_object();

/* Rendering */
public:
	void render();
private:
	bool update_draw = true;

private:
	std::vector<GameObject*> objects;
	//the camera at the zeroth index will always be the "main camera"
	std::vector<Camera*> cameras;
	std::vector<Light*> lights;

	/* Antuco Initalization */
public:
	//delete copy trait
	Antuco(const Antuco&) = delete;

	static Antuco& get_engine() {
		return Antuco_instance;
	}

	~Antuco();

private:
	Antuco();
	static Antuco Antuco_instance;
};



}
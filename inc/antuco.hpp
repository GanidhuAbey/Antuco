/* ----------------- antuco.hpp -------------------
 * wrapper for user-level interaction with engine.
 * ------------------------------------------------
*/

#pragma once

#include "window.hpp"
#include "world_objects.hpp"
#include "graphics.hpp"
#include "config.hpp"

#include <component.hpp>

#include <vector>

namespace tuco {

class Antuco {
	/* Window */
public:
	Window* init_window(int w, int h, const char* title);
	void init_graphics(RenderEngine api);
private:
	Window* pWindow;
	Graphics* p_graphics;
	RenderEngine api;
	
	/* World Objects */
public:
	DirectionalLight& create_spotlight(glm::vec3 light_pos, glm::vec3 light_target, glm::vec3 light_color,  glm::vec3 up, bool cast_shadows=false);
    PointLight& create_point_light(glm::vec3 pos, glm::vec3 colour);
	Camera* create_camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float yfov, float near, float far);
	GameObject* create_object();

/* Rendering */
public:
	void render();
private:
	bool update_draw = true;

private:
	//shared_ptr because main.cpp needs to access and modify game objects
	std::vector<std::unique_ptr<GameObject>> objects;
	//the camera at the zeroth index will always be the "main camera"
	std::vector<Camera*> cameras;

	std::vector<DirectionalLight> directional_lights;
    std::vector<PointLight> point_lights;
	std::vector<int> shadow_casters;

	std::unordered_map<uint32_t, std::vector<Component>> components;

	/* Antuco Initalization */
public:
	//delete copy trait
	Antuco(const Antuco&) = delete;

	void add_component(Component&& component, uint32_t entity_id);
	Component* get_component(uint32_t entity_id, uint32_t component_id);

	static Antuco& get_engine() {
		return Antuco_instance;
	}

	~Antuco();

private:
	Antuco();
	static Antuco Antuco_instance;
};



}

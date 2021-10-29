/* HANDLES ALL OBJECTS THAT WILL EXIST WITHIN THE RENDER WORLD */
//it would actually be pretty easy to seperate these into their own header files but right now they aren't big enough to warrant it
//having to navigate this file, i can say it will probably be easier to just seperate this into different files
#pragma once

#include "model.hpp"
#include "graphics.hpp"

#include <glm/glm.hpp>

namespace tuco {

class Light {
	friend class Antuco;
private:
	glm::vec3 position;
	glm::vec3 color;

	glm::mat4 world_to_light;
	glm::mat4 perspective;
private:
	Light(glm::vec3 light_pos, glm::vec3 light_color);
public:
	~Light();
	void update(glm::vec3 translation);
};

class Camera {
	friend class Antuco;
	friend class GraphicsImpl;
private:
	glm::mat4 modelToCamera;
	glm::mat4 cameraToScreen;

	glm::vec3 point_of_focus;

	glm::vec3 orientation;
private:
	Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float yfov, float aspect_ratio, float near, float far);
public:
	~Camera();
	void update(glm::vec3 camera_pos, glm::vec3 camera_face);
private:
	glm::mat4 construct_world_to_camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up);
	glm::mat4 perspective_projection(float angle, float aspect, float n, float f);
};

class GameObject {
	friend class Antuco;
	friend class GraphicsImpl;
private:
	glm::mat4 transform;
	bool update = true;
	Model object_model;
	size_t back_end_data = 0;
	uint32_t changed = 0;
private:
	GameObject();
public:
	~GameObject();
	void add_mesh(const std::string& fileName);
	void scale(glm::vec3 scale_vector);
	void translate(glm::vec3 t);
};

}
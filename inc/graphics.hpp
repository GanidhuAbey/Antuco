#pragma once

#pragma once

#include "window.hpp"
#include "mesh.hpp"

#include "world_objects.hpp"

#include <glm/glm.hpp>


namespace tuco {

class GraphicsImpl;

class Graphics {
private:
	GraphicsImpl* pGraphics;
public:
	Graphics(Window* pWindow);
	~Graphics();

public:
	void update_camera(glm::mat4 world_to_camera, glm::mat4 projection);
	void update_light(std::vector<tuco::Light*> lights);
	void update_draw(std::vector<GameObject*> game_objects);

};
}
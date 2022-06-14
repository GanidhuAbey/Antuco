#include "graphics.hpp"

#include "api_graphics.hpp"
#include "model.hpp"


using namespace tuco;

Graphics::Graphics(Window* pWindow) {
	p_graphics = std::make_unique<GraphicsImpl>(pWindow);
}

Graphics::~Graphics() {
}

void Graphics::update_camera(glm::mat4 world_to_camera, glm::mat4 projection, glm::vec4 pos) {
	p_graphics->update_camera(world_to_camera, projection, pos);
}

void Graphics::update_light(std::vector<Light*> lights, std::vector<int> shadow_indices) {
	//TODO: we aren't dealing with lighting all that seriously right now so we are just gonna disable this while we work on shadowmaps
	//		leaving this todo to comeback to this after i implemented multiple shadowmaps
	p_graphics->update_light(lights, shadow_indices);
}

void Graphics::update_draw(std::vector<GameObject*> game_object) {
	p_graphics->update_draw(game_object); //should probably send a pointer of this...
}

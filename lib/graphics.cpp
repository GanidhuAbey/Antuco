#include "graphics.hpp"

#include "api_graphics.hpp"
#include "model.hpp"


using namespace tuco;

Graphics::Graphics(Window* pWindow) {
	pGraphics = new GraphicsImpl(pWindow);
}

Graphics::~Graphics() {
	delete pGraphics;
}

void Graphics::update_camera(glm::mat4 world_to_camera, glm::mat4 projection) {
	pGraphics->update_camera(world_to_camera, projection);
}

void Graphics::update_light(std::vector<Light*> lights) {
	//TODO: we aren't dealing with lighting all that seriously right now so we are just gonna disable this while we work on shadowmaps
	//		leaving this todo to comeback to this after i implemented multiple shadowmaps
	pGraphics->update_light(lights);
}

void Graphics::update_draw(std::vector<GameObject*> game_object) {
	pGraphics->update_draw(game_object); //should probably send a pointer of this...
}
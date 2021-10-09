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

void Graphics::update_light(glm::vec3 color, glm::vec3 position) {
	pGraphics->update_light(glm::vec4(color, 0.0), glm::vec4(position, 1.0));
}

void Graphics::update_draw(std::vector<GameObject*> game_object) {
	pGraphics->update_draw(game_object); //should probably send a pointer of this...
}
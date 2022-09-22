/* --------------------- graphics.hpp -------------------------
 * Abstracts away the api level function required for rendering
 * ------------------------------------------------------------
*/
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
	std::unique_ptr<GraphicsImpl> p_graphics;
public:
	Graphics(Window* pWindow);
	~Graphics();

public:
	void update_camera(glm::mat4 world_to_camera, glm::mat4 projection, glm::vec4 pos);
	void update_light(
            std::vector<tuco::DirectionalLight> lights, 
            std::vector<int> shadow_indices);

	void update_draw(std::vector<std::unique_ptr<GameObject>>& game_objects, bool scene_change);

};
}

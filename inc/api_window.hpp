/* Uses an API to implement window functionality */

#pragma once

#include "window.hpp"
#include "antuco_enums.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

/* LIGHT ABSTRACTION FROM THE WINDOW LIBRARY FOR THE USER */
namespace tuco {

class WindowImpl {
private:
	GLFWwindow* apiWindow;
public:
	//handle window creation and window destruction
	WindowImpl(int w, int h, const char* title);
	~WindowImpl();

	//handle events
	bool check_window_status(WindowStatus windowEvent);

	//retrieve key inputs
	bool get_key_state(WindowInput windowInput);
	void get_mouse_pos(double* x_pos, double* y_pos);
	void lock_cursor();

	void create_vulkan_surface(VkInstance instance, VkSurfaceKHR* surface);
};

}
#include "api_window.hpp"

#include <stdexcept>

using namespace tuco;

/// <summary>
/// initalizes the backend window api
/// </summary>
/// <param name="w"> the width of the screen </param>
/// <param name="h"> the height of the screen </param>
/// <param name="title"> the title of the window </param>
WindowImpl::WindowImpl(int w, int h, const char* title) {
	//for now we can create a GLFW window but if we ever want tp use a different window API, the user won't have to relearn our API
	if (!glfwInit()) throw std::runtime_error("glfw failed to initialize!");

	//create the window
	//disable opengl context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	apiWindow = glfwCreateWindow(w, h, title, NULL, NULL);
}

WindowImpl::~WindowImpl() {}

/// <summary>
/// query the window status the user is asking for and check its status.
/// for more information on each window status, look at the WindowStatus enum
/// </summary>
/// <param name="windowEvent"> the window status that the user is querying for </param>
/// <returns> the state of the window event that the user is querying for </returns>
bool WindowImpl::check_window_status(WindowStatus windowEvent) {
	glfwPollEvents();
	switch (windowEvent) {
	case WindowStatus::CLOSE_REQUEST: return glfwWindowShouldClose(apiWindow);
	}

	printf("[ERROR] - The WindowStatus code is not queried by check_window_status() \n");

	return false;
}

/// <summary>
/// retrieves the mouse position of the cursor.
/// </summary>
/// <param name="x_pos"> the x position of the mouse (data in this variable will be overwritten) </param>
/// <param name="y_pos"> the y position of the mouse (data in this variable will be overwritten) </param>
void WindowImpl::get_mouse_pos(double* x_pos, double* y_pos) {
	glfwGetCursorPos(apiWindow, x_pos, y_pos);
}


/// <summary>
/// locks ths mouse cursor to the screen
/// </summary>
void WindowImpl::lock_cursor() {
	glfwSetInputMode(apiWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

/// <summary>
/// retrieves status of a keyboard input from user
/// </summary>
/// <param name="window_input"> key input being queried </param>
/// <returns> status of the window input that the user is querying for </returns>
bool WindowImpl::get_key_state(WindowInput window_input) {
	int glfw_input = translate_input(window_input);
	int state = glfwGetKey(apiWindow, glfw_input);

	if (state == GLFW_PRESS) {
		return true;
	}
	return false;
}


void WindowImpl::create_vulkan_surface(VkInstance instance, VkSurfaceKHR* surface) {
	VkResult surface_result = glfwCreateWindowSurface(instance, apiWindow, nullptr, surface);
	if (surface_result != VK_SUCCESS) {
		printf("[ERROR CODE: %d] - could not create vulkan surface on window", surface_result);
		throw std::runtime_error("");
	}
}

/// <summary>
/// closes window
/// </summary>
void WindowImpl::close() {
	glfwWindowShouldClose(apiWindow);
}

/// <summary>
/// translates key input to GLFW input
/// </summary>
/// <param name="input"> original WindowInput key code </param>
/// <returns> translated GLFW key code </returns
int WindowImpl::translate_input(WindowInput input) {
	switch(input) {
		case WindowInput::A:
			return GLFW_KEY_A;
		case WindowInput::W:
			return GLFW_KEY_W;
		case WindowInput::S:
			return GLFW_KEY_S;	
		case WindowInput::D:
			return GLFW_KEY_D;
		case WindowInput::X:
			return GLFW_KEY_X;
		case WindowInput::Q:
			return GLFW_KEY_Q;
	}
}

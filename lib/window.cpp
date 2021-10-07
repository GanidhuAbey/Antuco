#include "antuco.hpp"
#include "api_window.hpp"
#include "window.hpp"


using namespace tuco;

Window::Window(int w, int h, const char* title) {
	//initialize pWindow
	width = w;
	height = h;
	screen_title = title;
	pWindow = new WindowImpl(w, h, title);
}

Window::~Window() {}

bool Window::check_window_status(tuco::WindowStatus windowStatus) {
	return pWindow->check_window_status(windowStatus);
}

bool Window::get_key_state(tuco::WindowInput windowInput) {
	return pWindow->get_key_state(windowInput);
}

uint32_t Window::get_width() {
	return width;
}

uint32_t Window::get_height() {
	return height;
}

const char* Window::get_title() {
	return screen_title;
}
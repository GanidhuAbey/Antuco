/* A light wrapper that abstracts from the API-dependent window implementation. Essentialy makes the window functionality API agnostic */
#pragma once

#include <stdint.h>

namespace tuco {

enum WindowInput;
enum class WindowStatus;

class WindowImpl;

/* A light weight wrapper for the window library */
class Window {
	friend class Antuco;
	friend class GraphicsImpl;
private:
	uint32_t screen_width;
	uint32_t screen_height;
	const char* screen_title;
private:
	WindowImpl* pWindow;
	Window(int w, int h, const char* title);
public:
	~Window();

	bool check_window_status(WindowStatus);
	void lock_cursor();
	bool get_key_state(WindowInput);
	void get_mouse_pos(double* x_pos, double* y_pos);
	
//for now we'll have to do this...

public:
	uint32_t get_width();
	uint32_t get_height();
	const char* get_title();

};

}
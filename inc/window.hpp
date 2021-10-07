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
	uint32_t width;
	uint32_t height;
	const char* screen_title;
private:
	WindowImpl* pWindow;
	Window(int w, int h, const char* title);
public:
	~Window();

	bool get_key_state(WindowInput);
	bool check_window_status(WindowStatus);
	
//for now we'll have to do this...

public:
	uint32_t get_width();
	uint32_t get_height();
	const char* get_title();

};

}
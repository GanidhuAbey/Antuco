#pragma once

namespace tuco {
enum class WindowStatus {
	CLOSE_REQUEST, //status of the close button
};

//this almost always going to be a terrible idea but i took the enum values from GLFW and hard coded here...
enum WindowInput {
	W = 87,
	A = 65,
	S = 83,
	D = 68,
	X = 88,
};
}

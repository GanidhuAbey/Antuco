#include "world_objects.hpp"
#include "glm/glm.hpp"

using namespace tuco;

Camera::Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float yfov, float aspect_ratio, float near, float far) {
	orientation = up;
	//need to construct some matrices to represent this camera
	modelToCamera = construct_world_to_camera(eye, target, up);
	cameraToScreen = perspective_projection(yfov, aspect_ratio, near, far);
}
Camera::~Camera() {}


void Camera::update(glm::vec3 camera_pos, glm::vec3 camera_face) {
	modelToCamera = construct_world_to_camera(camera_pos, camera_face, orientation);
}


/// <summary>
/// constructs a matrix to translate objects from world space to space relative to the camera. since the camera can't exist and we can only ever see straight down the -z axis
/// we must transform the objects such that when seen from the -z axis they appear as if they are seen by the imaginary camera
/// </summary>
/// <param name="eye"> the location of the camera </param>
/// <param name="target"> vector representing the direction the cameara is facing </param>
/// <param name="up"> the orientation of the camera </param>
/// <returns></returns>
glm::mat4 Camera::construct_world_to_camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up) {
	//create three orthogonal vectors to define a transformation that will take us from camera space to world space
	glm::vec3 direction = target + eye;
	glm::vec3 look_at = glm::normalize(direction - eye);
	glm::vec3 right = glm::normalize(glm::cross(look_at, up));
	up = glm::cross(right, look_at);

	look_at = -look_at;

	//these three vectors can define the transformation from camera to world, but we need to inverse it to get world to camera
	//this inverse will need to be multiplied by a translation that accounts for the location of the camera
	glm::mat4 inverse = {
		right.x, right.y, right.z, -glm::dot(right, eye),
		up.x, up.y, up.z, -glm::dot(up, eye),
		look_at.x, look_at.y, look_at.z, -glm::dot(look_at, eye),
		0, 0, 0, 1
	};

	return glm::transpose(inverse);
}

/// <summary>
/// calculates a perspective view using matrices. works by projecting objects in "camera" space into a screen that has a very small depth.
/// the camera viewport is modeled as a frustum (a pyramid that converges at the point of the camera). This viewport helps create the perspective illusion,
/// and by using some trigonometry we can construct this matrix that defines the transformation from camera space to this thin screen.
/// </summary>
/// <param name="angle"> the angle between the top-end and the bottom-end of the window screen </param>
/// <param name="aspect"> the aspect ratio of the window </param>
/// <param name="n"> the near plane (make this value slightly above 0 always) </param>
/// <param name="f"> the far plane (determines how far the player can see) </param>
/// <returns></returns>
glm::mat4 Camera::perspective_projection(float angle, float aspect, float n, float f) {
	double c = 1.0 / (glm::tan(angle / 2));

	printf("the value of the first input is: %f \n", aspect);

	glm::mat4 projection = {
		c / aspect, 0, 0, 0,
		0, c, 0, 0,
		0, 0, -(f + n) / (f - n), -(2 * f * n) / (f - n),
		0, 0, -1, 0,
	};

	return glm::transpose(projection);
}
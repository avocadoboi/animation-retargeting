// A few GLFW abstractions.

#ifndef ANIMATION_RETARGETING_TESTING_GLFW_HPP
#define ANIMATION_RETARGETING_TESTING_GLFW_HPP

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace testing {
namespace glfw {

struct Instance {
	Instance() {
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	}
	~Instance() {
		glfwTerminate();
	}

	Instance(Instance const&) = delete;
	Instance& operator=(Instance const&) = delete;
	
	Instance(Instance&&) = delete;
	Instance& operator=(Instance&&) = delete;
};

class InputState {
private:
	GLFWwindow* window_;

	glm::vec2 cursor_position_{};
	glm::vec2 cursor_movement_{};

public:
	InputState(GLFWwindow* const window = nullptr) :
		window_{window}
	{
		if (window != nullptr) {
			update();
		}
	}

	void update() {
		double x, y;
		glfwGetCursorPos(window_, &x, &y);

		cursor_movement_ = glm::vec2{x, y} - cursor_position_;
		cursor_position_ = glm::vec2{x, y};
	}

	glm::vec2 cursor_position() const {
		return cursor_position_;
	}

	glm::vec2 cursor_movement() const {
		return cursor_movement_;
	}

	bool is_key_down(int const key) const {
		return glfwGetKey(window_, key) == GLFW_PRESS;
	}
	bool is_mouse_button_down(int const button) const {
		return glfwGetMouseButton(window_, button) == GLFW_PRESS;
	}
};

} // namespace glfw
} // namespace testing

#endif
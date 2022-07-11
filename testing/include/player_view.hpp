#ifndef ANIMATION_RETARGETING_TESTING_PLAYER_VIEW_HPP
#define ANIMATION_RETARGETING_TESTING_PLAYER_VIEW_HPP

#include "glfw.hpp"

#include <glm/ext.hpp>

namespace testing {

class PlayerView {
private:
	static constexpr auto movement_acceleration = 0.05f;
	static constexpr auto movement_friction = 0.2f;
	static constexpr auto mouse_sensitivity = 0.004f;

	glm::mat4 view_matrix_{1.f};
	
	glm::vec2 rotation_{};
	glm::vec3 position_{};

	glm::vec3 velocity_{};

	void update_view_() {
		view_matrix_ = glm::mat4{1.f};
		view_matrix_ = glm::rotate(view_matrix_, rotation_.x, glm::vec3{0.f, 1.f, 0.f});
		view_matrix_ = glm::rotate(view_matrix_, rotation_.y, glm::vec3{glm::cos(rotation_.x), 0.f, glm::sin(rotation_.x)});
		view_matrix_ = glm::translate(view_matrix_, -position_);
	}
	
public:
	explicit PlayerView(glm::vec3 const start_position) :
		position_{start_position}
	{
		update_view_();
	}

	void update_movement(glfw::InputState const& input) 
	{
		if (input.is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT)) {
			rotation_ += input.cursor_movement()*mouse_sensitivity;
		}

		if (input.is_key_down(GLFW_KEY_W)) {
			velocity_ += glm::vec3{glm::sin(rotation_.x), 0.f, -glm::cos(rotation_.x)}*movement_acceleration;
		}
		if (input.is_key_down(GLFW_KEY_S)) {
			velocity_ -= glm::vec3{glm::sin(rotation_.x), 0.f, -glm::cos(rotation_.x)}*movement_acceleration;
		}
		if (input.is_key_down(GLFW_KEY_A)) {
			velocity_ -= glm::vec3{glm::cos(rotation_.x), 0.f, glm::sin(rotation_.x)}*movement_acceleration;
		}
		if (input.is_key_down(GLFW_KEY_D)) {
			velocity_ += glm::vec3{glm::cos(rotation_.x), 0.f, glm::sin(rotation_.x)}*movement_acceleration;
		}
		if (input.is_key_down(GLFW_KEY_Q)) {
			velocity_ -= glm::vec3{0.f, movement_acceleration, 0.f};
		}
		if (input.is_key_down(GLFW_KEY_E)) {
			velocity_ += glm::vec3{0.f, movement_acceleration, 0.f};
		}
		velocity_ *= 1 - movement_friction;

		position_ += velocity_;

		update_view_();
	}

	glm::mat4 view_matrix() const {
		return view_matrix_;
	}
};

} // namespace testing

#endif
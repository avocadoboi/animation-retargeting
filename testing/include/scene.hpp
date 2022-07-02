#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "animated_character.hpp"
#include "player_view.hpp"

namespace testing {

class Scene {
private:
	static constexpr auto player_position = glm::vec3{0.f, 8.f, 0.f};

	static constexpr auto animation_path = "testing/data/animations/walking.fbx";

	std::array<AnimatedCharacter, 4> characters_{
		AnimatedCharacter{Model{"testing/data/models/human.fbx", Texture{"testing/data/models/human.png"}}, animation_path},
		AnimatedCharacter{Model{"testing/data/models/archer.fbx", Texture{"testing/data/models/archer.png"}}, animation_path},
		AnimatedCharacter{Model{"testing/data/models/vampire.fbx", Texture{"testing/data/models/vampire.png"}}, animation_path},
		AnimatedCharacter{Model{"testing/data/models/Praying.fbx", Texture{"testing/data/models/human.png"}}, animation_path},
	};
	
	PlayerView view_{player_position};

	void update_projection_(glm::vec2 const size) {
		auto const new_projection = glm::perspective(glm::radians(50.f), size.x/size.y, 0.1f, 100.f);

		for (auto& character : characters_) {
			character.projection_matrix(new_projection);
		}
	}

public:
	Scene(glm::vec2 const size) {
		update_projection_(size);

		constexpr auto spacing = 15.f;

		auto x = -(static_cast<float>(characters_.size()) - 1.f)*spacing/2.f;

		for (auto& character : characters_) {
			character.restart_animation();
			character.position_scale(glm::vec3{x, 0.f, -30.f}, 1.f/15.f);
			x += spacing;
		}
	}

	void handle_resize(glm::vec2 const new_size) {
		if (new_size != glm::vec2{}) {
			update_projection_(new_size);
		}
	}

	void update(glfw::InputState const& input_state) {
		view_.update_movement(input_state);

		for (auto& character : characters_) {
			character.update_animation();
		}
	}

	void draw() {
		for (auto& character : characters_) {
			character.draw_model(view_.view_matrix());
		}

		glClear(GL_DEPTH_BUFFER_BIT);

		for (auto& character : characters_) {
			character.draw_skeleton(view_.view_matrix());
		}
	}
};

} // namespace testing

#endif
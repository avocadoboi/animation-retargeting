#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "model.hpp"
#include "player_view.hpp"

namespace testing {

constexpr auto vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

out vec3 normal;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    normal = in_normal;
    uv = in_uv;
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
)";

constexpr auto fragment_shader = R"(
#version 330 core
in vec3 normal;
in vec2 uv;

out vec4 fragment_color;

uniform sampler2D diffuse_texture;

void main()
{
    fragment_color = texture(diffuse_texture, uv)*mix(0.6, 1, normal.y*0.5 + 0.5);
}
)";

class Scene {
private:
    static constexpr auto player_position = glm::vec3{0.f, 8.f, 0.f};

    Model model_{"testing/data/models/vampire.fbx", Texture{"testing/data/models/vampire.png"}};
    ShaderProgram shader_{vertex_shader, fragment_shader};
    PlayerView view_{player_position};

    void update_projection_(glm::vec2 const size) {
        shader_.use();
        shader_.set_mat4("projection", glm::perspective(glm::radians(50.f), size.x/size.y, 0.1f, 100.f));
    }

public:
    Scene(glm::vec2 const size) {
        update_projection_(size);
        shader_.set_mat4("view", glm::mat4{1.f});
        shader_.set_mat4("model", glm::scale(glm::translate(glm::mat4{1.f}, glm::vec3{0.f, 0.f, -20.f}), glm::vec3{1.f/15.f}));
        shader_.set_int("diffuse_texture", 0);
    }

    void handle_resize(glm::vec2 const new_size) {
        update_projection_(new_size);
    }

    void update(glfw::InputState const& input_state) {
        view_.update_movement(input_state);
    }

    void draw() {
        shader_.use();
        shader_.set_mat4("view", view_.view_matrix());

        model_.draw();
    }
};

} // namespace testing

#endif
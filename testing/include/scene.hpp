#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "animation.hpp"
#include "model.hpp"
#include "player_view.hpp"

namespace testing {

/*
constexpr auto vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

const int max_bone_influence = 4;
layout (location = 3) in uvec4 bone_ids;
layout (location = 4) in vec4 bone_weights;

out vec3 normal;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int max_bone_count = 256;
uniform mat4 bone_matrices[max_bone_count];

void main()
{
    uv = in_uv;

    vec4 total_position = vec4(0.f);
    vec3 total_normal = vec3(0.f);

    for (int i = 0; i < max_bone_influence; ++i)
    {
        if (bone_weights[i] == 0.f) {
            break;
        }
        if (bone_ids[i] >= max_bone_count) {
            total_position = vec4(in_pos, 1.f);
            break;
        }

        mat4 transform = bone_weights[i] * bone_matrices[bone_ids[i]];
        total_position += transform * vec4(in_pos, 1.f);
        total_normal += mat3(transform) * in_normal;
    }

    gl_Position = projection * view * model * total_position;
    normal = normalize(total_normal);
}
)";
*/
constexpr auto vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

const int max_bone_influence = 4;
layout (location = 3) in uvec4 bone_ids;
layout (location = 4) in vec4 bone_weights;

out vec3 normal;
out vec2 uv;
out float weight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform uint bone_index;

void main()
{
    uv = in_uv;
    normal = in_normal;
    weight = 0.f;

    for (int i = 0; i < max_bone_influence; ++i)
    {
        if (bone_weights[i] == 0.f) {
            break;
        }
        if (bone_ids[i] == bone_index) {
            weight = bone_weights[i];
            break;
        }
    }

    gl_Position = projection * view * model * vec4(in_pos, 1.f);
}
)";

constexpr auto fragment_shader = R"(
#version 330 core
in vec3 normal;
in vec2 uv;
in float weight;

out vec4 fragment_color;

uniform sampler2D diffuse_texture;

void main()
{
    //fragment_color = texture(diffuse_texture, uv)*mix(0.6, 1, normal.y*0.5 + 0.5);
    fragment_color = mix(texture(diffuse_texture, uv)*mix(0.6, 1, normal.y*0.5 + 0.5), vec4(0.f, 1.f, 0.f, 0.f), weight);
}
)";

class Scene {
private:
    static constexpr auto player_position = glm::vec3{0.f, 8.f, 0.f};

    Model model_{"testing/data/models/vampire.fbx", Texture{"testing/data/models/vampire.png"}};
    ShaderProgram shader_{vertex_shader, fragment_shader};
    PlayerView view_{player_position};

    Animation animation_{"testing/data/animations/walking.fbx"};

    GLuint current_bone_index_{};
    std::uint64_t frame_count_{};

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

        if (frame_count_++ % 30 == 0) {
            shader_.set_uint("bone_index", current_bone_index_ = (current_bone_index_ + 1) % model_.skeleton().bone_count());
        }

        model_.draw();
    }
};

} // namespace testing

#endif
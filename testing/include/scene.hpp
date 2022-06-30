#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "animation.hpp"
#include "model.hpp"
#include "player_view.hpp"

#include <fmt/format.h>

namespace testing {

constexpr auto model_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

layout (location = 3) in uvec4 bone_ids;
layout (location = 4) in vec4 bone_weights;

out vec3 normal;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const uint max_bone_count = 128u;
uniform mat4 bone_matrices[max_bone_count];

void main()
{
    uv = in_uv;
    normal = in_normal;

    mat4 bone_transform = bone_matrices[bone_ids[0]] * bone_weights[0];
    bone_transform += bone_matrices[bone_ids[1]] * bone_weights[1];
    bone_transform += bone_matrices[bone_ids[2]] * bone_weights[2];
    bone_transform += bone_matrices[bone_ids[3]] * bone_weights[3];

    gl_Position = projection * view * model * bone_transform * vec4(in_pos, 1.f);
    //normal = normalize(total_normal);
}
)";

constexpr auto model_fragment_shader = R"(
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

constexpr auto skeleton_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in uint in_bone_id;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const uint max_bone_count = 128u;
uniform mat4 bone_matrices[max_bone_count];

void main() {
    gl_Position = projection * view * model * bone_matrices[in_bone_id] * vec4(in_pos, 1.f);
}
)";

constexpr auto skeleton_fragment_shader = R"(
#version 330 core

out vec4 fragment_color;

void main() {
    fragment_color = vec4(0.f, 1.f, 0.f, 0.7f);
}
)";

inline std::string bone_matrix_uniform_name(Bone::Id const id)
{
    return "bone_matrices[" + std::to_string(id) + "]";
    // return fmt::format("bone_matrices[{}]", id);
}

class Scene {
private:
    static constexpr auto player_position = glm::vec3{0.f, 8.f, 0.f};

    Model model_{"testing/data/models/archer.fbx", Texture{"testing/data/models/archer.png"}};
    ShaderProgram model_shader_{model_vertex_shader, model_fragment_shader};
    Animation animation_{"testing/data/animations/walking.fbx", &model_.skeleton()};

    SkeletonMesh skeleton_mesh_{model_.skeleton()};
    ShaderProgram skeleton_shader_{skeleton_vertex_shader, skeleton_fragment_shader};
    
    PlayerView view_{player_position};

    void update_projection_(glm::vec2 const size) {
        auto const new_projection = glm::perspective(glm::radians(50.f), size.x/size.y, 0.1f, 100.f);
        
        model_shader_.use();
        model_shader_.set_mat4("projection", new_projection);

        skeleton_shader_.use();
        skeleton_shader_.set_mat4("projection", new_projection);
    }

public:
    Scene(glm::vec2 const size) {
        update_projection_(size);

        auto const model_transform = glm::translate(glm::mat4{1.f}, glm::vec3{0.f, 0.f, -20.f}) * glm::scale(glm::mat4{1.f}, glm::vec3{1.f/15.f});
        model_shader_.use();
        model_shader_.set_mat4("view", glm::mat4{1.f});
        model_shader_.set_mat4("model", model_transform);
        model_shader_.set_int("diffuse_texture", 0);

        skeleton_shader_.use();
        skeleton_shader_.set_mat4("view", glm::mat4{1.f});
        skeleton_shader_.set_mat4("model", model_transform);
    }

    void handle_resize(glm::vec2 const new_size) {
        if (new_size != glm::vec2{}) {
            update_projection_(new_size);
        }
    }

    void update(glfw::InputState const& input_state) {
        view_.update_movement(input_state);
        animation_.update_bone_matrices();
    }

    void draw() {
        model_shader_.use();
        model_shader_.set_mat4("view", view_.view_matrix());
        for (auto const& bone : model_.skeleton().bones()) {
            model_shader_.set_mat4(bone_matrix_uniform_name(bone.id).c_str(), bone.animation_transform);
        }
        model_.draw();

        glClear(GL_DEPTH_BUFFER_BIT);

        skeleton_shader_.use();
        skeleton_shader_.set_mat4("view", view_.view_matrix());
        for (auto const& bone : model_.skeleton().bones()) {
            skeleton_shader_.set_mat4(bone_matrix_uniform_name(bone.id).c_str(), bone.animation_transform);
        }
        skeleton_mesh_.draw();
    }
};

} // namespace testing

#endif
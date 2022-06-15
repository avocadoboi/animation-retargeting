#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "model.hpp"

#include <glm/ext.hpp>

namespace testing {

constexpr auto vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = in_uv;
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
}
)";

constexpr auto fragment_shader = R"(
#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D diffuse_texture;

void main()
{
    FragColor = texture(diffuse_texture, TexCoords);
}
)";

class Scene {
private:
    Model model_{"testing/data/models/archer.fbx", Texture{"testing/data/models/archer.png"}};
    ShaderProgram shader_{vertex_shader, fragment_shader};

public:
    Scene(glm::vec2 const size)
    {
        shader_.use();
        shader_.set_mat4("projection", glm::perspective(glm::radians(45.f), size.x/size.y, 0.1f, 100.f));
        shader_.set_mat4("view", glm::mat4{1.f});
        shader_.set_mat4("model", glm::scale(glm::translate(glm::mat4{1.f}, glm::vec3{0.f, -5.f, -20.f}), glm::vec3{1.f/15.f}));
        shader_.set_int("diffuse_texture", 0);
    }

    void draw() const {
        shader_.use();
        model_.draw();
    }
};

} // namespace testing

#endif
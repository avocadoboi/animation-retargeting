#ifndef ANIMATION_RETARGETING_TESTING_SCENE_HPP
#define ANIMATION_RETARGETING_TESTING_SCENE_HPP

#include "model.hpp"

namespace testing {

// constexpr auto 

class Scene {
private:
    Model model_{"testing/data/models/archer/archer.fbx", Texture{"testing/data/models/archer/archer.png"}};
    ShaderProgram shader_{"", ""};

public:
    void draw() const {
        model_.draw();
    }
};

} // namespace testing

#endif
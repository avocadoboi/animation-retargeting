#ifndef ANIMATION_RETARGETING_TESTING_APP_HPP
#define ANIMATION_RETARGETING_TESTING_APP_HPP

#include "scene.hpp"

#include <GLFW/glfw3.h>

namespace testing {

struct GlfwInstance {
    GlfwInstance() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    ~GlfwInstance() {
        glfwTerminate();
    }

    GlfwInstance(GlfwInstance const&) = delete;
    GlfwInstance& operator=(GlfwInstance const&) = delete;
    
    GlfwInstance(GlfwInstance&&) = delete;
    GlfwInstance& operator=(GlfwInstance&&) = delete;
};

class App {
private:
    static constexpr auto window_size = glm::vec<2, int>{700, 600};
    static constexpr auto window_title = "Animation retargeting";

    GlfwInstance instance_;

    GLFWwindow* window_;

    std::unique_ptr<Scene> scene_;

    void process_input_() const {
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window_, true);
        }
    }

public:
    App() :
        window_{glfwCreateWindow(window_size.x, window_size.y, window_title, nullptr, nullptr)}
    {
        if (not window_) {
            throw std::runtime_error{"Failed to create window."};
        }
        
        glfwMakeContextCurrent(window_);

        if (not gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            throw std::runtime_error{"Failed to load OpenGL."};
        }

        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow*, int const width, int const height) {
            glViewport(0, 0, width, height);
        });

        scene_ = std::make_unique<Scene>();
    }

    void run() const {
        while (not glfwWindowShouldClose(window_)) {
            process_input_();

            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            scene_->draw();
            
            glfwSwapBuffers(window_);
            glfwPollEvents();
        }
    }

};

} // namespace testing

#endif
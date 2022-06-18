#ifndef ANIMATION_RETARGETING_TESTING_APP_HPP
#define ANIMATION_RETARGETING_TESTING_APP_HPP

#include "scene.hpp"

namespace testing {

class App {
private:
    static constexpr auto window_size = glm::vec<2, int>{700, 600};
    static constexpr auto window_title = "Animation retargeting";

    glfw::Instance instance_;

    GLFWwindow* window_;
    glfw::InputState input_state_;

    std::unique_ptr<Scene> scene_;

    void initialize_opengl_()
    {
        glfwMakeContextCurrent(window_);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            throw std::runtime_error{"Failed to load OpenGL."};
        }

        glEnable(GL_DEPTH_TEST);

        glfwSwapInterval(1);
    }

public:
    App()
    {
        glfwWindowHint(GLFW_SAMPLES, 32);

        window_ = glfwCreateWindow(window_size.x, window_size.y, window_title, nullptr, nullptr);

        if (!window_) {
            throw std::runtime_error{"Failed to create window."};
        }

        input_state_ = glfw::InputState{window_};

        initialize_opengl_();

        scene_ = std::make_unique<Scene>(window_size);

        glfwSetWindowUserPointer(window_, scene_.get());

        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* const window, int const width, int const height) {
            glViewport(0, 0, width, height);
            static_cast<Scene*>(glfwGetWindowUserPointer(window))->handle_resize({width, height});
        });

    }
    ~App() {
        glfwDestroyWindow(window_);
    }

    App(App const&) = delete;
    App const& operator=(App const&) = delete;
    App(App&&) = delete;
    App const& operator=(App&&) = delete;

    void run() {
        while (!glfwWindowShouldClose(window_)) {
            input_state_.update();
            scene_->update(input_state_);
            
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            scene_->draw();
            
            glfwSwapBuffers(window_);
            glfwPollEvents();
        }
    }

};

} // namespace testing

#endif
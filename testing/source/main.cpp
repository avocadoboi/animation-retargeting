#include "model.hpp"

#include <GLFW/glfw3.h>

#include <iostream>

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

class Window {
private:
    GLFWwindow* window_;

    void process_input_() const
    {
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window_, true);
        }
    }

public:
    Window(glm::vec<2, int> const size, char const* const title) :
        window_{glfwCreateWindow(size.x, size.y, title, nullptr, nullptr)}
    {
        if (not window_) {
            throw std::runtime_error{"Failed to create window."};
        }
        
        glfwMakeContextCurrent(window_);

        if (not gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            throw std::runtime_error{"Failed to load OpenGL."};
        }

        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* const window, int width, int height) {
            glViewport(0, 0, width, height);
        });
    }

    // Window(Window const&) = delete;
    // Window const& operator=(Window const&) = delete;
    
    // Window(Window&&) = delete;
    // Window const& operator=(Window&&) = delete;

    void run() const 
    {
        while (not glfwWindowShouldClose(window_)) {
            process_input_();

            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glfwSwapBuffers(window_);
            glfwPollEvents();
        }
    }

};

} // namespace testing

int main() {
    auto const instance = testing::GlfwInstance{};

    auto const window = testing::Window{{800, 600}, "Hello"};

    window.run();
}

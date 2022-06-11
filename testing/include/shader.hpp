#ifndef ANIMATION_RETARGETING_TESTING_SHADER_HPP
#define ANIMATION_RETARGETING_TESTING_SHADER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <gsl/gsl>

#include <array>
#include <iostream>

namespace testing {

namespace impl 
{

class Shader {
private:
    GLuint id_;

public:
    Shader(gsl::span<char const> const code, GLenum const type) :
        id_{glCreateShader(type)}
    {
        auto const shader_id = glCreateShader(type);

        auto const* const code_data = code.data();
        auto const code_length = static_cast<GLint>(code.size());
        glShaderSource(shader_id, 1, &code_data, &code_length);

        glCompileShader(shader_id);

        // Print any compilation errors.

        auto succeeded = GLint{};
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &succeeded);

        if (not succeeded) {
            std::array<char, 512> info_log;
            glGetShaderInfoLog(shader_id, info_log.size(), nullptr, info_log.data());

            std::cout << "Shader compilation failed:\n" << info_log.data() << '\n';
        }
    }

    ~Shader() {
        glDeleteShader(id_);
    }

    Shader(Shader const&) = delete;
    Shader const& operator=(Shader const&) = delete;

    Shader(Shader&&) = delete;
    Shader const& operator=(Shader&&) = delete;

    GLuint id() const {
        return id_;
    }
};

} // namespace impl

class ShaderProgram {
private:
    GLuint id_;

public:
    ShaderProgram(gsl::span<char const> const vertex_code, gsl::span<char const> const fragment_code) 
    {
        auto const vertex_shader = impl::Shader{vertex_code, GL_VERTEX_SHADER};
        auto const fragment_shader = impl::Shader{fragment_code, GL_VERTEX_SHADER};

        id_ = glCreateProgram();

        glAttachShader(id_, vertex_shader.id());
        glAttachShader(id_, fragment_shader.id());
        
        glLinkProgram(id_);

        auto succeeded = GLint{};
        glGetProgramiv(id_, GL_LINK_STATUS, &succeeded);

        if (not succeeded) {
            std::array<char, 512> info_log;
            glGetProgramInfoLog(id_, info_log.size(), nullptr, info_log.data());

            std::cout << "Shader linking failed:\n" << info_log.data() << '\n';
        }
    }

    GLuint id() const {
        return id_;
    }

    void set_bool(char const* const name, bool const value) {
        glUniform1i(glGetUniformLocation(id_, name), static_cast<GLint>(value));
    }
    void set_int(char const* const name, int const value) {
        glUniform1i(glGetUniformLocation(id_, name), static_cast<GLint>(value));
    }
    void set_float(char const* const name, float const value) {
        glUniform1f(glGetUniformLocation(id_, name), static_cast<GLfloat>(value));
    }
};

} // namespace testing 

#endif
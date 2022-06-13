#ifndef ANIMATION_RETARGETING_TESTING_SHADER_HPP
#define ANIMATION_RETARGETING_TESTING_SHADER_HPP

#include <fmt/format.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <gsl/span>

#include <array>

namespace testing {

namespace detail {

class Shader {
private:
    GLuint id_;

public:
    Shader(gsl::span<char const> const code, GLenum const type) :
        id_{glCreateShader(type)}
    {
        auto const* const code_data = code.data();
        auto const code_length = static_cast<GLint>(code.size());
        glShaderSource(id_, 1, &code_data, &code_length);

        glCompileShader(id_);

        // Print any compilation errors.

        auto succeeded = GLint{};
        glGetShaderiv(id_, GL_COMPILE_STATUS, &succeeded);

        if (not succeeded) {
            std::array<char, 512> info_log;
            glGetShaderInfoLog(id_, info_log.size(), nullptr, info_log.data());

            fmt::print("Shader compilation failed:\n{}\n", info_log.data());
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

} // namespace detail

class ShaderProgram {
private:
    GLuint id_;

public:
    ShaderProgram(gsl::span<char const> const vertex_code, gsl::span<char const> const fragment_code) :
        id_{glCreateProgram()}
    {
        auto const vertex_shader = detail::Shader{vertex_code, GL_VERTEX_SHADER};
        auto const fragment_shader = detail::Shader{fragment_code, GL_VERTEX_SHADER};

        glAttachShader(id_, vertex_shader.id());
        glAttachShader(id_, fragment_shader.id());
        
        glLinkProgram(id_);

        auto succeeded = GLint{};
        glGetProgramiv(id_, GL_LINK_STATUS, &succeeded);

        if (not succeeded) {
            std::array<char, 512> info_log;
            glGetProgramInfoLog(id_, info_log.size(), nullptr, info_log.data());

            fmt::print("Shader linking failed:\n{}\n", info_log.data());
        }
    }

    ~ShaderProgram() {
        glDeleteProgram(id_);
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
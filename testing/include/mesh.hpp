#ifndef ANIMATION_RETARGETING_TESTING_MESH_HPP
#define ANIMATION_RETARGETING_TESTING_MESH_HPP

#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace testing {

template<typename T>
std::size_t vector_byte_size(std::vector<T> const& vector) {
    return vector.size()*sizeof(T);
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coordinates;
};

class Mesh {
private:
    std::vector<Vertex> vertices_;
    std::vector<std::size_t> indices_;

    GLuint texture_id_;

    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;

public:
    Mesh(std::vector<Vertex> vertices, std::vector<std::size_t> indices, GLuint texture_id) :
        vertices_{std::move(vertices)}, indices_{std::move(indices)}, texture_id_{texture_id}
    {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vector_byte_size(vertices_), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vector_byte_size(vertices_), vertices_.data(), GL_STATIC_DRAW);

        // Vertex positions.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

        // Vertex normals.
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, normal)));

        // Vertex texture coordinates.
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, texture_coordinates)));
    }

    void draw() const {
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, texture_id_);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};

} // namespace testing

#endif

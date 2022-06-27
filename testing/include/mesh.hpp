#ifndef ANIMATION_RETARGETING_TESTING_MESH_HPP
#define ANIMATION_RETARGETING_TESTING_MESH_HPP

#include "shader.hpp"
#include "skeleton.hpp"

#include <vector>

namespace testing {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coordinates;

    static constexpr auto max_bone_influence = std::size_t{4};
    std::array<Bone::Id, max_bone_influence> bone_ids{};
    std::array<float, max_bone_influence> bone_weights{};

    void add_bone(Bone::Id const id, float const weight)
    {
        if (weight < 0.01f) {
            return;
        }
        for (auto i = std::size_t{}; i < max_bone_influence; ++i) {
            if (bone_weights[i] == 0.f) {
                bone_ids[i] = id;
                bone_weights[i] = weight;
                return;
            }
        }
        fmt::print("Too many bone influences!!!\n");
    }
};

class Mesh {
private:
    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;

    GLuint texture_id_;

    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;

    void create_gpu_buffers_()
    {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        // Vertices.
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, util::vector_byte_size(vertices_), vertices_.data(), GL_STATIC_DRAW);

        // Vertex indices.
        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, util::vector_byte_size(indices_), indices_.data(), GL_STATIC_DRAW);
    }

    void set_vertex_attributes_()
    {
        // Vertex positions.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

        // Vertex normals.
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, normal)));

        // Vertex texture coordinates.
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, texture_coordinates)));

        // Influencing bone IDs.
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, Vertex::max_bone_influence, GL_UNSIGNED_INT, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, bone_ids)));
        static_assert(std::is_same<Bone::Id, GLuint>::value);

        // Influencing bone weights.
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, Vertex::max_bone_influence, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, bone_weights)));
    }

public:
    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, GLuint const texture_id) :
        vertices_{std::move(vertices)}, indices_{std::move(indices)}, texture_id_{texture_id}
    {
        create_gpu_buffers_();
        set_vertex_attributes_();
    }

    void draw() const 
    {
        if (texture_id_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_id_);
        }

        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};

} // namespace testing

#endif

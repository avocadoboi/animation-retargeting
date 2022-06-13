#ifndef ANIMATION_RETARGETING_TESTING_MODEL_HPP
#define ANIMATION_RETARGETING_TESTING_MODEL_HPP

#include "mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace testing {

glm::vec3 assimp_to_glm_vec(aiVector3D const vector) {
    return glm::vec3{vector.x, vector.y, vector.z};
}
glm::vec2 assimp_to_glm_vec(aiVector2D const vector) {
    return glm::vec2{vector.x, vector.y};
}

namespace detail {

std::vector<Vertex> load_mesh_vertices(aiMesh const* const mesh)
{
    auto vertices = std::vector<Vertex>{};
    
    for (auto i = std::size_t{}; i < mesh->mNumVertices; ++i)
    {
        auto vertex = Vertex{};

        vertex.position = assimp_to_glm_vec(mesh->mVertices[i]);

        if (mesh->HasNormals()) {
            vertex.normal = assimp_to_glm_vec(mesh->mNormals[i]);
        }

        if (mesh->mTextureCoords[0])
        {
            auto const texture_coordinates = mesh->mTextureCoords[0][i];
            vertex.texture_coordinates = glm::vec2{texture_coordinates.x, texture_coordinates.y};
        }

        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<std::size_t> load_mesh_indices(aiMesh const* const mesh)
{
    auto indices = std::vector<std::size_t>{};

    for (auto const face : gsl::make_span(mesh->mFaces, mesh->mNumFaces))
    {
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    return indices;
}

} // namespace detail

class Model {
private:
    std::vector<Mesh> meshes_;

    void load_mesh_(aiMesh const* const mesh, aiScene const* const scene)
    {
        auto vertices = detail::load_mesh_vertices(mesh);

        auto indices = detail::load_mesh_indices(mesh);

        
        // meshes_.emplace_back(std::move(vertices), std::move(indices), texture_id);
    }

public:
    Model(std::string const& path) 
    {
        auto importer = Assimp::Importer{};
        auto const* const scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (not scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || not scene->mRootNode)
        {
            throw std::runtime_error{fmt::format("Failed to import model: {}\n", importer.GetErrorString())};
        }

        for (auto* const mesh : gsl::make_span(scene->mMeshes, scene->mNumMeshes))
        {
            load_mesh_(mesh, scene);
        }
    }
    
    void draw() const {
        for (auto const& mesh : meshes_) {
            mesh.draw();
        }
    }
};

} // namespace testing

#endif
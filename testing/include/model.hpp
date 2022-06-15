#ifndef ANIMATION_RETARGETING_TESTING_MODEL_HPP
#define ANIMATION_RETARGETING_TESTING_MODEL_HPP

#include "fbx.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <fmt/format.h>

namespace testing {

class Model {
private:
    std::vector<Mesh> meshes_;
    Texture texture_;

    void load_mesh_(FbxMesh* const mesh)
    {
        auto* const control_points = mesh->GetControlPoints();
        auto const& normals = mesh->GetElementNormal()->GetDirectArray();
        auto const& texture_coordinates = mesh->GetElementUV()->GetDirectArray();

        auto vertices = std::vector<Vertex>(mesh->GetControlPointsCount());

        for (auto i = std::size_t{}; i < vertices.size(); ++i)
        {
            auto const point = control_points[i];
            vertices[i].position = glm::vec3{
                point.mData[0],
                point.mData[1],
                point.mData[2],
            };

            auto const normal = normals.GetAt(static_cast<int>(i));
            vertices[i].normal = glm::vec3{
                normal.mData[0],
                normal.mData[1],
                normal.mData[2],
            };

            auto const uv = texture_coordinates.GetAt(static_cast<int>(i));
            vertices[i].texture_coordinates = glm::vec2{
                uv.mData[0],
                uv.mData[1],
            };
        }

        auto const& index_array = mesh->GetElementUV()->GetIndexArray();

        auto indices = std::vector<GLuint>(index_array.GetCount());

        for (auto i = std::size_t{}; i < indices.size(); ++i)
        {
            indices[i] = index_array.GetAt(static_cast<int>(i));
        }
        
        meshes_.emplace_back(std::move(vertices), std::move(indices), texture_.id());
    }

    void load_meshes_(FbxNode* const node)
    {
        if (auto* const attribute = node->GetNodeAttribute())
        {
            if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                auto* const mesh = static_cast<FbxMesh*>(attribute);
                
                load_mesh_(mesh);
            }
        }

        for (auto i = int{}; i < node->GetChildCount(); ++i)
        {
            load_meshes_(node->GetChild(i));
        }
    }

public:
    Model() = default;

    Model(char const* const fbx_path, Texture texture) :
        texture_{std::move(texture)}
    {
        auto manager = fbx::create<FbxManager>();
        
        // manager->SetIOSettings(fbx::create<FbxIOSettings>(manager.get(), IOSROOT).get());
        
        auto scene = fbx::import_scene(manager.get(), fbx_path);

        auto* const root_node = scene->GetRootNode();
        if (not root_node) {
            throw std::runtime_error{"An FBX scene did not contain a root node."};
        }

        for (auto i = int{}; i < root_node->GetChildCount(); ++i)
        {
            load_meshes_(root_node->GetChild(i));
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
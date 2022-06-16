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

    void load_mesh_(FbxMesh const* const mesh)
    {
        auto const* const vertices_source = mesh->GetControlPoints();

        auto const* const normal_layer = mesh->GetElementNormal();
        auto const* const uv_layer = mesh->GetElementUV();
        
        auto vertices = std::vector<Vertex>(mesh->GetControlPointsCount());

        for (auto i = std::size_t{}; i < vertices.size(); ++i)
        {
            vertices[i].position = fbx::to_glm_vec(vertices_source[i]);

            if (normal_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByControlPoint)
            {
                vertices[i].normal = fbx::layer_element_at(normal_layer, static_cast<int>(i));
            }
            
            if (uv_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByControlPoint)
            {
                vertices[i].texture_coordinates = fbx::layer_element_at(uv_layer, static_cast<int>(i));
            }
        }

        auto const* const indices_source = mesh->GetPolygonVertices();
        auto indices = std::vector<GLuint>(mesh->GetPolygonVertexCount());

        for (auto i = std::size_t{}; i < indices.size(); ++i)
        {
            indices[i] = static_cast<GLuint>(indices_source[i]);

            if (normal_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByPolygonVertex)
            {
                vertices[indices[i]].normal = fbx::layer_element_at(normal_layer, static_cast<int>(i));
            }
            
            if (uv_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByPolygonVertex)
            {
                vertices[indices[i]].texture_coordinates = fbx::layer_element_at(uv_layer, static_cast<int>(i));
            }
        }
        
        meshes_.emplace_back(std::move(vertices), std::move(indices), texture_.id());
    }

    void load_meshes_(FbxNode const* const node)
    {
        if (auto const* const attribute = node->GetNodeAttribute())
        {
            if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                load_mesh_(static_cast<FbxMesh const*>(attribute));
            }
        }

        for (auto i = int{}; i < node->GetChildCount(); ++i)
        {
            load_meshes_(node->GetChild(i));
        }
    }

    static fbx::Unique<FbxIOSettings> create_import_settings_(FbxManager* const manager)
    {
        auto settings = fbx::create<FbxIOSettings>(manager, IOSROOT);
        settings->SetBoolProp(IMP_FBX_ANIMATION, false);
        settings->SetBoolProp(IMP_FBX_AUDIO, false);
        settings->SetBoolProp(IMP_FBX_BINORMAL, false);
        settings->SetBoolProp(IMP_FBX_CHARACTER, false);
        settings->SetBoolProp(IMP_FBX_CONSTRAINT, false);
        settings->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        settings->SetBoolProp(IMP_FBX_MATERIAL, false);
        settings->SetBoolProp(IMP_FBX_TANGENT, false);
        settings->SetBoolProp(IMP_FBX_TEXTURE, false);
        settings->SetBoolProp(IMP_FBX_VERTEXCOLOR, false);
        return settings;
    }

public:
    Model() = default;

    Model(char const* const fbx_path, Texture texture) :
        texture_{std::move(texture)}
    {
        auto manager = fbx::create<FbxManager>();
        
        auto settings = create_import_settings_(manager.get());
        manager->SetIOSettings(settings.get());
        
        auto scene = fbx::import_scene(manager.get(), fbx_path);

        FbxGeometryConverter{manager.get()}.Triangulate(scene.get(), true);

        auto const* const root_node = scene->GetRootNode();

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
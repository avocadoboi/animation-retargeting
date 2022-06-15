#ifndef ANIMATION_RETARGETING_TESTING_MODEL_HPP
#define ANIMATION_RETARGETING_TESTING_MODEL_HPP

#include "fbx.hpp"
#include "mesh.hpp"
#include "texture.hpp"

namespace testing {

class Model {
private:
    std::vector<Mesh> meshes_;
    Texture texture_;

public:
    Model() = default;

    Model(std::string const& fbx_path, Texture texture) :
        texture_{std::move(texture)}
    {
        auto manager = fbx::create<FbxManager>();
        
        auto settings = fbx::create<FbxIOSettings>(manager.get(), IOSROOT);
        manager->SetIOSettings(settings.get());

        auto importer = fbx::create<FbxImporter>(manager.get(), "");

        if (not importer->Initialize(fbx_path.c_str(), -1, manager->GetIOSettings()))
        {
            throw std::runtime_error{"Failed to initialize FBX importer."};
        }

        auto scene = fbx::create<FbxScene>(manager.get(), "my_scene");
        importer->Import(scene.get());
    }
    
    void draw() const {
        for (auto const& mesh : meshes_) {
            mesh.draw();
        }
    }
};

} // namespace testing

#endif
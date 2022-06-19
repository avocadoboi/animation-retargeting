#ifndef ANIMATION_RETARGETING_TESTING_ANIMATION_HPP
#define ANIMATION_RETARGETING_TESTING_ANIMATION_HPP

#include "fbx.hpp"

#include <fmt/format.h>

namespace testing {

class Animation {
private:

    static fbx::Unique<FbxIOSettings> create_import_settings_(FbxManager* const manager)
    {
        auto settings = fbx::create<FbxIOSettings>(manager, IOSROOT);
        settings->SetBoolProp(IMP_FBX_AUDIO, false);
        settings->SetBoolProp(IMP_FBX_BINORMAL, false);
        settings->SetBoolProp(IMP_FBX_CHARACTER, false);
        settings->SetBoolProp(IMP_FBX_CONSTRAINT, false);
        settings->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        settings->SetBoolProp(IMP_FBX_MATERIAL, false);
        settings->SetBoolProp(IMP_FBX_MODEL, false);
        settings->SetBoolProp(IMP_FBX_NORMAL, false);
        settings->SetBoolProp(IMP_FBX_TANGENT, false);
        settings->SetBoolProp(IMP_FBX_TEXTURE, false);
        settings->SetBoolProp(IMP_FBX_VERTEXCOLOR, false);
        return settings;
    }

public:
    Animation(char const* const fbx_path)
    {
        auto manager = fbx::create<FbxManager>();
        
        auto settings = create_import_settings_(manager.get());
        manager->SetIOSettings(settings.get());
        
        auto scene = fbx::import_scene(manager.get(), fbx_path);

        auto const* const root_node = scene->GetRootNode();

        if (!root_node) {
            throw std::runtime_error{"An FBX scene did not contain a root node."};
        }

    }
};

} // namespace testing

#endif
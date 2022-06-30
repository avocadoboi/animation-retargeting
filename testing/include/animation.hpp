#ifndef ANIMATION_RETARGETING_TESTING_ANIMATION_HPP
#define ANIMATION_RETARGETING_TESTING_ANIMATION_HPP

#include "fbx.hpp"
#include "skeleton.hpp"

#include <fmt/format.h>
#include <glm/ext.hpp>

#include <chrono>

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

    using Clock_ = std::chrono::steady_clock;

    Skeleton* skeleton_;
    std::chrono::time_point<Clock_> start_time_{Clock_::now()};

    void load_animation_(FbxNode* const node, FbxAnimLayer* const animation_layer)
    {
		if (auto const* const attribute = node->GetNodeAttribute())
		{
			if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
                if (auto* const bone = skeleton_->bone_by_name(node->GetNameOnly())) 
                {
                    bone->scale_track = AnimationTrack<glm::vec3>{animation_layer, node->LclScaling};
                    bone->rotation_track = AnimationTrack<glm::quat>{animation_layer, node->LclRotation};
                    bone->translation_track = AnimationTrack<glm::vec3>{animation_layer, node->LclTranslation};
                }
			}
		}
		
		for (auto i = int{}; i < node->GetChildCount(); ++i)
		{
			load_animation_(node->GetChild(i), animation_layer);
		}	
    }

public:
    Animation(char const* const fbx_path, Skeleton* const skeleton) :
        skeleton_{skeleton}
    {
        auto manager = fbx::create<FbxManager>();
        
        auto settings = create_import_settings_(manager.get());
        manager->SetIOSettings(settings.get());
        
        auto scene = fbx::import_scene(manager.get(), fbx_path);

        FbxAxisSystem::OpenGL.DeepConvertScene(scene.get());

        auto* const root_node = scene->GetRootNode();

        if (!root_node) {
            throw std::runtime_error{"An FBX scene did not contain a root node."};
        }

        auto const stack_count = scene->GetSrcObjectCount(FbxCriteria::ObjectType(FbxAnimStack::ClassId));
        for (auto stack_index = 0; stack_index < stack_count; ++stack_index)
        {
            auto const* const animation_stack = static_cast<FbxAnimStack const*>(scene->GetSrcObject(FbxCriteria::ObjectType(FbxAnimStack::ClassId), stack_index));

            auto const layer_count = animation_stack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId));
            for (auto layer_index = 0; layer_index < layer_count; ++layer_index)
            {
                auto* const animation_layer = static_cast<FbxAnimLayer*>(animation_stack->GetMember(FbxCriteria::ObjectType(FbxAnimLayer::ClassId), layer_index));
                load_animation_(root_node, animation_layer);
            }
        }
        
    }

    void restart() {
        start_time_ = Clock_::now();
    }

    void update_bone_matrices()
    {
        auto const time = std::chrono::duration_cast<Seconds>(Clock_::now() - start_time_)*0.5f;

        // Hack, remove later
        if (time > skeleton_->bones()[0].translation_track.duration()) {
            restart();
        }

        for (auto& bone : skeleton_->bones())
        {
            auto const local_scale = bone.scale_track.evaluate(time, bone.default_scale);
            auto const local_rotation = bone.rotation_track.evaluate(time, bone.default_rotation);
            auto const local_translation = bone.translation_track.evaluate(time, bone.default_translation);
            // auto const local_scale = bone.default_scale;
            // auto const local_rotation = bone.default_rotation;
            // auto const local_translation = bone.default_translation;
            
            auto const local_transform = bone.calculate_local_transform(local_scale, local_rotation, local_translation);
            
            if (bone.parent) {
                bone.global_transform = bone.parent->global_transform * local_transform;
            }
            else {
                bone.global_transform = local_transform;
            }

            bone.animation_transform = bone.global_transform * bone.inverse_bind_transform;
        }
    }
};

} // namespace testing

#endif
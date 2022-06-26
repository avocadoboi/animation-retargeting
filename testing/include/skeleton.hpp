#ifndef ANIMATION_RETARGETING_TESTING_SKELETON_HPP
#define ANIMATION_RETARGETING_TESTING_SKELETON_HPP

#include "fbx.hpp"
#include "util.hpp"

#include <fmt/format.h>
#include <glm/ext.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <vector>

namespace testing {

using Seconds = std::chrono::duration<float>;

template<typename T>
struct Keyframe {
    Seconds time;
    T value;
};

template<typename T>
class AnimationTrack {
private:
    std::vector<Keyframe<T>> keyframes_;

    template<typename U = T>
    static auto create_value_(float const x, float const y, float const z)
        -> std::enable_if_t<std::is_same<U, glm::vec3>::value, U>
    {
        return glm::vec3{x, y, z};
    }
    template<typename U = T>
    static auto create_value_(float const x, float const y, float const z)
        -> std::enable_if_t<std::is_same<U, glm::quat>::value, U>
    {
        return glm::quat{glm::radians(glm::vec3{x, y, z})};
    }

public:
    AnimationTrack() = default;

    explicit AnimationTrack(std::vector<Keyframe<T>> keyframes) :
        keyframes_{std::move(keyframes)}
    {}
    AnimationTrack(FbxAnimLayer* const layer, FbxPropertyT<FbxDouble3>& property)
    {
        auto const* const curve_x = property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X);
        auto const* const curve_y = property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y);
        auto const* const curve_z = property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z);

        if (curve_x && curve_y && curve_z)
        {
            auto const key_count = curve_x->KeyGetCount();

            keyframes_.reserve(key_count);

            for (auto i = int{}; i < key_count; ++i)
            {
                keyframes_.push_back(Keyframe<T>{
                    Seconds{curve_x->KeyGetTime(i).GetSecondDouble()}, 
                    create_value_(
                        curve_x->KeyGetValue(i),
                        curve_y->KeyGetValue(i),
                        curve_z->KeyGetValue(i)
                    )
                });
            }
        }
    }

    bool is_empty() const {
        return keyframes_.empty();
    }

    T evaluate(Seconds const time) const {
        assert(!keyframes_.empty());
        
        auto const end_key = std::lower_bound(keyframes_.begin(), keyframes_.end(), time, 
            [](Keyframe<T> const key, Seconds const time) { return key.time < time; });

        if (end_key == keyframes_.begin()) {
            return keyframes_.front().value;
        }
        if (end_key == keyframes_.end()) {
            return keyframes_.back().value;
        }

        auto const start_key = end_key - 1;

        return util::map(start_key->time.count(), end_key->time.count(), start_key->value, end_key->value, time.count());
    }

    T evaluate(Seconds const time, T const default_value) const {
        if (is_empty()) {
            return default_value;
        }
        return evaluate(time);
    }
};

struct Bone {
    using Id = unsigned int;

    Bone const* parent;
    std::string name;

    glm::mat4 inverse_bind_transform;
    glm::mat4 global_transform;

    glm::vec3 default_translation;
    glm::vec3 default_scale;
    glm::quat default_rotation;

    AnimationTrack<glm::vec3> translation_track;
    AnimationTrack<glm::vec3> scale_track;
    AnimationTrack<glm::quat> rotation_track;
};

class Skeleton {
private:
    std::vector<Bone> bones_;

    void add_bone_(FbxNode* const bone_node, Bone const* parent)
    {
        auto bone = Bone{};
        bone.parent = parent;
        bone.name = bone_node->GetNameOnly();

        bone.default_translation = util::fbx_to_glm(bone_node->LclTranslation.Get());
        bone.default_scale = util::fbx_to_glm(bone_node->LclScaling.Get());
        bone.default_rotation = glm::quat{glm::radians(util::fbx_to_glm(bone_node->LclRotation.Get()))};
        
        // auto real_local_inverse_transform = bone_node->EvaluateLocalTransform().Inverse();

        auto const local_inverse_transform = glm::inverse(util::translation_scale_rotation(
            bone.default_translation, bone.default_scale, bone.default_rotation));

        if (bone.parent) {
            bone.inverse_bind_transform = local_inverse_transform * bone.parent->inverse_bind_transform;
        }
        else {
            bone.inverse_bind_transform = local_inverse_transform;
        }
        
        bones_.push_back(std::move(bone));

        parent = &bones_.back();

        for (auto i = int{}; i < bone_node->GetChildCount(); ++i)
        {
            add_bone_(bone_node->GetChild(i), parent);
        }    
    }
    
    auto bone_iterator_by_name_(char const* const name) const
    {
        return std::find_if(bones_.begin(), bones_.end(), [&](Bone const& bone) { return bone.name == name; });
    }
    auto bone_iterator_by_name_(char const* const name)
    {
        return std::find_if(bones_.begin(), bones_.end(), [&](Bone const& bone) { return bone.name == name; });
    }
    
public:
    Skeleton() {
        bones_.reserve(256);
    }

    void load_from_fbx_node(FbxNode* const root_node) 
    {
        if (auto* const root_bone = fbx::find_root_bone(root_node)) 
        {
            add_bone_(root_bone, nullptr);
        }
        else {
            throw std::runtime_error{"Skeleton did not have root bone."};
        }
    }

    Bone const* bone_by_name(char const* const name) const
    {
        auto const pos = bone_iterator_by_name_(name);
        if (pos != bones_.end())
        {
            return &*pos;
        }
        return nullptr;
    }
    Bone* bone_by_name(char const* const name)
    {
        auto pos = bone_iterator_by_name_(name);
        if (pos != bones_.end())
        {
            return &*pos;
        }
        return nullptr;
    }

    Bone::Id bone_id_by_name(char const* const name) const {
        return static_cast<Bone::Id>(bone_iterator_by_name_(name) - bones_.begin());
    }

    Bone const* bone_by_id(Bone::Id const id) const {
        return &bones_[id];
    }
    Bone* bone_by_id(Bone::Id const id) {
        return &bones_[id];
    }

    std::size_t bone_count() const {
        return bones_.size();
    }

    std::vector<Bone> const& bones() const {
        return bones_;
    }
    std::vector<Bone>& bones() {
        return bones_;
    }
};

} // namespace testing

#endif
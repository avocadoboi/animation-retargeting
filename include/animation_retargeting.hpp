#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <string>
#include <vector>
#include <string_view>

#include <fmt/format.h>

namespace animation_retargeting {

struct PoseBone {
    std::string name;
    
    static constexpr auto no_parent = static_cast<std::size_t>(-1);
    std::size_t parent_index{no_parent};

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
};

struct Pose {
    std::vector<PoseBone> bones;
};

struct AnimatedBone {
    std::vector<glm::vec3> scales;
    std::vector<glm::quat> rotations;
    std::vector<glm::vec3> translations;
};

struct Animation {
    std::vector<AnimatedBone> bones;
};

struct RetargetResult {
    Animation animation;
    Pose bind_pose;
};

inline Pose bake_rotations(Pose pose) 
{
    for (auto& bone : pose.bones) {
        if (bone.parent_index != bone.no_parent) {
            auto const parent_rotation = pose.bones[bone.parent_index].rotation;
            bone.translation = glm::rotate(parent_rotation, bone.translation);
            bone.rotation = parent_rotation * bone.rotation;
        }
    }
    for (auto& bone : pose.bones) {
        bone.rotation = glm::identity<glm::quat>();
    }
    return pose;
}

inline RetargetResult retarget(Animation source_animation, Pose const& source_bind_pose, Pose target_bind_pose)
{
    assert(source_animation.bones.size() == source_bind_pose.bones.size());

    target_bind_pose = bake_rotations(std::move(target_bind_pose));
    
    auto result = RetargetResult{};
    for (auto& target_pose_bone : target_bind_pose.bones) 
    {
        auto const source_pos = std::find_if(source_bind_pose.bones.begin(), source_bind_pose.bones.end(), [&](auto const& x) { return x.name == target_pose_bone.name; });
        
        if (source_pos == source_bind_pose.bones.end()) {
            result.bind_pose.bones.push_back(std::move(target_pose_bone));
            result.animation.bones.push_back(AnimatedBone{});
        }
        else {
            auto const translation_rotation_offset = glm::rotation(glm::normalize(source_pos->translation), glm::normalize(target_pose_bone.translation));
            auto const scale_factor = std::sqrt(glm::length2(target_pose_bone.translation) / glm::length2(source_pos->translation));

            auto& animated_bone = source_animation.bones[source_pos - source_bind_pose.bones.begin()];
            for (auto& translation : animated_bone.translations) {         
                translation = glm::rotate(translation_rotation_offset, translation * scale_factor);
            }
            result.animation.bones.push_back(std::move(animated_bone));

            result.bind_pose.bones.push_back(std::move(target_pose_bone));
        }
    }
    return result;
}

} // namespace animation_retargeting

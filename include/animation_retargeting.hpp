#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace animation_retargeting {

struct PoseBone {
    std::string name;
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


inline Animation retarget(Animation source_animation, Pose const& source_bind_pose, Pose const& target_bind_pose)
{
    assert(source_animation.bones.size() == source_bind_pose.bones.size());
    
    auto result = Animation{};
    for (auto const& bone : target_bind_pose.bones) 
    {
        auto const source_pos = std::find_if(source_bind_pose.bones.begin(), source_bind_pose.bones.end(), [&](auto const& x) { return x.name == bone.name; });
        
        if (source_pos == source_bind_pose.bones.end()) {
            result.bones.push_back(AnimatedBone{{bone.scale}, {bone.rotation}, {bone.translation}});
        }
        else {
            result.bones.push_back(std::move(source_animation.bones[source_pos - source_bind_pose.bones.begin()]));
        }
    }
    return result;
}

} // namespace animation_retargeting

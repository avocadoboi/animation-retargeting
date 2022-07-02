#ifndef ANIMATION_RETARGETING_TESTING_UTIL_HPP
#define ANIMATION_RETARGETING_TESTING_UTIL_HPP

#include <fbxsdk.h>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace testing {

namespace util {

template<typename T>
std::size_t vector_byte_size(std::vector<T> const& vector) {
    return vector.size()*sizeof(T);
}

template<typename T, typename U>
U map(T const from_start, T const from_end, U const to_start, U const to_end, T const from)
{
    return to_start + (to_end - to_start)*(from - from_start)/(from_end - from_start);
}
template<typename T>
glm::quat map(T const from_start, T const from_end, glm::quat const to_start, glm::quat const to_end, T const from)
{
    return glm::slerp(to_start, to_end, (from - from_start)/(from_end - from_start));
}

inline glm::mat4 fbx_to_glm(FbxAMatrix const& m) {
    return glm::make_mat4(static_cast<double const*>(m));
}
inline glm::vec3 fbx_to_glm(FbxDouble3 const vector) {
    return glm::vec3{vector.mData[0], vector.mData[1], vector.mData[2]};
}
inline glm::vec3 fbx_to_glm(FbxDouble4 const vector) {
    return glm::vec3{vector.mData[0], vector.mData[1], vector.mData[2]};
}

inline glm::mat4 euler_angles_to_mat4_xyz(glm::vec3 const euler) {
    return glm::eulerAngleXYZ(euler.x, euler.y, euler.z);
}

inline std::string trimmed_bone_name(FbxNode const* const bone_node) {
    auto const name = bone_node->GetNameOnly();
    return std::string{name.Mid(name.Find(':') + 1)};
}

// glm::quat euler_angles_to_quat(glm::vec3 const angles, FbxEuler::EOrder const order) 
// {
//     switch (order) {
//         case FbxEuler::EOrder::eOrderXYZ:
//             return glm::quat{angles};
//     }
// }

} // namespace util

} // namespace testing

#endif
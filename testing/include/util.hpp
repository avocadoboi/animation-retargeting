#ifndef ANIMATION_RETARGETING_TESTING_UTIL_HPP
#define ANIMATION_RETARGETING_TESTING_UTIL_HPP

#include <fbxsdk.h>
#include <glm/ext.hpp>

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

inline glm::vec3 fbx_to_glm(FbxDouble3 const vector) {
    return glm::vec3{vector.mData[0], vector.mData[1], vector.mData[2]};
}

inline glm::mat4 translation_scale_rotation(glm::vec3 const translation, glm::vec3 const scale, glm::quat const rotation) {
    return glm::translate(glm::mat4_cast(rotation)*glm::scale(glm::mat4{1.f}, scale), translation);
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
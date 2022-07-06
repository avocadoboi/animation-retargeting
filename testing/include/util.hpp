#ifndef ANIMATION_RETARGETING_TESTING_UTIL_HPP
#define ANIMATION_RETARGETING_TESTING_UTIL_HPP

#include <fbxsdk.h>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
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

inline bool string_ends_with(std::string const& string, std::string const& end) {
	return string.size() >= end.size() && !string.compare(string.size() - end.size(), end.size(), end);
}

inline bool is_end_bone(std::string const& bone_name) {
	return util::string_ends_with(bone_name, "_end") || util::string_ends_with(bone_name, "_End");
}

// glm::quat euler_angles_to_quat(glm::vec3 const angles, FbxEuler::EOrder const order) 
// {
//     switch (order) {
//         case FbxEuler::EOrder::eOrderXYZ:
//             return glm::quat{angles};
//     }
// }

template<typename T, std::size_t capacity_>
class StaticVector {
private:
	std::array<T, capacity_> array_;
	std::size_t size_{};

public:
	static constexpr auto capacity = capacity_;

	std::size_t size() const {
		return size_;
	}

	void push_back(T&& element) {
		assert(size_ < capacity_);        
		array_[size_++] = std::move(element);
	}
	void pop_back() {
		assert(size_);
		--size_;
	}

	T const& operator[](std::size_t const index) const {
		assert(index < size_);
		return array_[index];
	}
	T& operator[](std::size_t const index) {
		assert(index < size_);
		return array_[index];
	}

	T const& back() const {
		assert(size_);
		return array_[size_ - 1];
	}
	T& back() {
		assert(size_);
		return array_[size_ - 1];
	}

	auto begin() const {
		return array_.begin();
	}
	auto begin() {
		return array_.begin();
	}

	auto end() const {
		return array_.begin() + size_;
	}
	auto end() {
		return array_.begin() + size_;
	}
};

template<typename Index_>
class Indices {
private:
	Index_ size_;
	
public:
	constexpr explicit Indices(Index_ const size) :
		size_{size}
	{}
	template<typename Container_>
	constexpr explicit Indices(Container_ const& container) :
		size_{container.size()}
	{}

	struct Iterator {
		Index_ index;

		constexpr Iterator& operator++() {
			++index;
			return *this;
		}
		constexpr bool operator==(Iterator const other) const {
			return index == other.index;
		}
		constexpr bool operator!=(Iterator const other) const {
			return index != other.index;
		}
		constexpr Index_ operator*() const {
			return index;
		}
	};

	constexpr Iterator begin() const {
		return Iterator{};
	}
	constexpr Iterator end() const {
		return Iterator{size_};
	}
};

template<typename Index_, typename = std::enable_if_t<std::is_integral_v<Index_>>>
constexpr Indices<Index_> indices(Index_ const size) {
	return Indices<Index_>{size};
}
template<typename Container_, typename = std::enable_if_t<!std::is_integral_v<Container_>>>
constexpr Indices<std::size_t> indices(Container_ const& container) {
	return Indices<std::size_t>{container};
}

} // namespace util

} // namespace testing

#endif
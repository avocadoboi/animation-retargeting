#ifndef ANIMATION_RETARGETING_TESTING_SKELETON_HPP
#define ANIMATION_RETARGETING_TESTING_SKELETON_HPP

#include "fbx.hpp"
#include "util.hpp"

#include <fmt/format.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/gtx/component_wise.hpp>

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
	static auto create_value_(FbxDouble3 const vector)
		-> std::enable_if_t<std::is_same<U, glm::vec3>::value, U>
	{
		return util::fbx_to_glm(vector);
	}
	template<typename U = T>
	static auto create_value_(FbxDouble3 const vector)
		-> std::enable_if_t<std::is_same<U, glm::quat>::value, U>
	{
		return glm::quat{glm::radians(util::fbx_to_glm(vector))};
	}

public:
	AnimationTrack() = default;

	explicit AnimationTrack(std::vector<Keyframe<T>> keyframes) :
		keyframes_{std::move(keyframes)}
	{}
	AnimationTrack(FbxAnimLayer* const layer, FbxPropertyT<FbxDouble3>& property)
	{
		auto const curves = std::array<FbxAnimCurve const*, 3>{
			property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X),
			property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y),
			property.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z),
		};

		if (curves[0] || curves[1] || curves[2])
		{
			auto const get_key_count = [](auto const* const curve) { return curve ? curve->KeyGetCount() : 0; };
			auto const key_counts = glm::ivec3{get_key_count(curves[0]), get_key_count(curves[1]), get_key_count(curves[2])};

			keyframes_.reserve(glm::compMax(key_counts));

			// The current keyframe index for each curve/channel.
			auto cursors = glm::ivec3{};

			while (cursors != key_counts)
			{
				// Pick the curve/channel with the next keyframe time.
				auto const channel = std::min({0u, 1u, 2u}, [&](auto const a, auto const b) {
					return cursors[a] != key_counts[a] ? cursors[b] != key_counts[b] ? 
						curves[a]->KeyGetTime(cursors[a]) < curves[b]->KeyGetTime(cursors[b]) : a : b;
				});
				
				auto const time = curves[channel]->KeyGetTime(cursors[channel]);

				// Increment the keyframe cursor(s).
				++cursors[channel];
				for (auto const i : {0u, 1u, 2u}) {
					if (i != channel && cursors[i] != key_counts[i] && curves[i]->KeyGetTime(cursors[i]).GetMilliSeconds() <= time.GetMilliSeconds() + 1) {
						++cursors[i];
					}
				}

				keyframes_.push_back(Keyframe<T>{
					Seconds{time.GetSecondDouble()},
					create_value_(property.EvaluateValue(time))
				});
			}
		}
	}

	bool is_empty() const {
		return keyframes_.empty();
	}

	Seconds duration() const {
		return keyframes_.empty() ? Seconds{} : keyframes_.back().time;
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
	Id id;

	glm::mat4 inverse_bind_transform;
	glm::mat4 global_transform;
	glm::mat4 animation_transform;

	glm::mat4 pre_scaling;
	glm::mat4 pre_rotation;
	glm::mat4 pre_translation;
	glm::mat4 post_translation;

	glm::vec3 default_scale;
	glm::quat default_rotation;
	glm::vec3 default_translation;

	AnimationTrack<glm::vec3> scale_track;
	AnimationTrack<glm::quat> rotation_track;
	AnimationTrack<glm::vec3> translation_track;

	glm::mat4 calculate_local_transform(glm::vec3 const scale, glm::quat const rotation, glm::vec3 const translation) const
	{
		return post_translation * glm::translate(glm::mat4{1.f}, translation) * pre_translation * glm::mat4_cast(rotation) * pre_rotation * glm::scale(glm::mat4{1.f}, scale) * pre_scaling;
	}
};

class Skeleton {
private:
	using Bones_ = util::StaticVector<Bone, 256>;
	std::unique_ptr<Bones_> bones_ = std::make_unique<Bones_>();
	glm::mat4 root_transform_;

	void add_bone_(FbxNode* const bone_node, Bone const* parent)
	{
		auto bone = Bone{};
		bone.parent = parent;
		bone.name = util::trimmed_bone_name(bone_node);
		bone.id = static_cast<Bone::Id>(bones_->size());

		bone.default_scale = util::fbx_to_glm(bone_node->LclScaling.Get());
		bone.default_rotation = glm::quat{glm::radians(util::fbx_to_glm(bone_node->LclRotation.Get()))};
		bone.default_translation = util::fbx_to_glm(bone_node->LclTranslation.Get());

		/*
			From the FBX SDK docs:
			World = ParentWorld * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1
			Here, I put Sp-1 into bone.pre_scaling, Rpost * Rp-1 * Soff * Sp into bone.pre_rotation, and Roff * Rp * Rpre into bone.pre_translation.
			Notice that me and the FBX SDK use different pre/post terms... I think mine are correct because the right side of a matrix
			multiplication is the transform that is applied first. T * R * S means scaling happens first, then rotation, then translation.
		*/

		auto const scaling_pivot = util::fbx_to_glm(bone_node->GetScalingPivot(FbxNode::eSourcePivot));
		bone.pre_scaling = glm::translate(glm::mat4{1.f}, -scaling_pivot);

		auto const rotation_pivot = util::fbx_to_glm(bone_node->GetRotationPivot(FbxNode::eSourcePivot));
		auto const post_rotation = util::euler_angles_to_mat4_xyz(glm::radians(util::fbx_to_glm(bone_node->GetPostRotation(FbxNode::eSourcePivot))));
		bone.pre_rotation = post_rotation * glm::translate(glm::mat4{1.f}, -rotation_pivot + util::fbx_to_glm(bone_node->GetScalingOffset(FbxNode::eSourcePivot)) + scaling_pivot);

		auto const pre_rotation = util::euler_angles_to_mat4_xyz(glm::radians(util::fbx_to_glm(bone_node->GetPreRotation(FbxNode::eSourcePivot))));
		bone.pre_translation = glm::translate(glm::mat4{1.f}, util::fbx_to_glm(bone_node->GetRotationOffset(FbxNode::eSourcePivot)) + rotation_pivot) * pre_rotation;

		// Add the parent node's global transform if this is a root bone.
		if (parent) {
			bone.post_translation = glm::mat4{1.f};
		}
		else {
			bone.post_translation = util::fbx_to_glm(bone_node->GetParent()->EvaluateGlobalTransform());
		}

        auto const local_inverse_transform = glm::inverse(bone.calculate_local_transform(bone.default_scale, bone.default_rotation, bone.default_translation));

        if (parent) {
            bone.inverse_bind_transform = local_inverse_transform * parent->inverse_bind_transform;
        }
        else {
            bone.inverse_bind_transform = local_inverse_transform;
        }

		bones_->push_back(std::move(bone));

		parent = &bones_->back();

		for (auto i = int{}; i < bone_node->GetChildCount(); ++i)
		{
			add_bone_(bone_node->GetChild(i), parent);
		}    
	}
	
	auto bone_iterator_by_name_(char const* const name) const
	{
		return std::find_if(bones_->begin(), bones_->end(), [&](Bone const& bone) { return bone.name == name; });
	}
	auto bone_iterator_by_name_(char const* const name)
	{
		return std::find_if(bones_->begin(), bones_->end(), [&](Bone const& bone) { return bone.name == name; });
	}
	
public:
	void load_from_fbx_node(FbxNode* const node) 
	{
		if (auto const* const attribute = node->GetNodeAttribute())
		{
			if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				add_bone_(node, nullptr);
				return;
			}
		}
		
		for (auto i = int{}; i < node->GetChildCount(); ++i)
		{
			load_from_fbx_node(node->GetChild(i));
		}		
	}

	Bone const* bone_by_name(char const* const name) const
	{
		auto const pos = bone_iterator_by_name_(name);
		if (pos != bones_->end()) {
			return &*pos;
		}
		return nullptr;
	}
	Bone* bone_by_name(char const* const name)
	{
		auto pos = bone_iterator_by_name_(name);
		if (pos != bones_->end()) {
			return &*pos;
		}
		return nullptr;
	}

	Bone::Id bone_id_by_name(char const* const name) const {
		return static_cast<Bone::Id>(bone_iterator_by_name_(name) - bones_->begin());
	}

	Bone const* bone_by_id(Bone::Id const id) const {
		return &(*bones_)[id];
	}
	Bone* bone_by_id(Bone::Id const id) {
		return &(*bones_)[id];
	}

	std::size_t bone_count() const {
		return bones_->size();
	}

	auto const& bones() const {
		return *bones_;
	}
	auto& bones() {
		return *bones_;
	}
};

struct SkeletonVertex {
	glm::vec3 position;
	Bone::Id bone_id;
};

class SkeletonMesh {
private:
	std::vector<SkeletonVertex> vertices_;
	std::vector<GLuint> indices_;

	GLuint vao_;
	GLuint vbo_;
	GLuint ebo_;

	void load_skeleton_(Skeleton const& skeleton) 
	{
		vertices_.reserve(skeleton.bone_count());
		indices_.reserve(skeleton.bone_count()*2);
		
		for (auto const& bone : skeleton.bones()) 
		{
			if (bone.parent) {
				indices_.push_back(bone.parent->id);
				indices_.push_back(bone.id);
			}

			vertices_.push_back(SkeletonVertex{glm::inverse(bone.inverse_bind_transform) * glm::vec4{0.f, 0.f, 0.f, 1.f}, bone.id});
		}
	}

	void create_gpu_buffers_()
	{
		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);

		// Vertices.
		glGenBuffers(1, &vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, util::vector_byte_size(vertices_), vertices_.data(), GL_STATIC_DRAW);

		// Vertex indices.
		glGenBuffers(1, &ebo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, util::vector_byte_size(indices_), indices_.data(), GL_STATIC_DRAW);
	}

	void set_vertex_attributes_()
	{
		// Bone vertex positions.
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletonVertex), nullptr);

		// Bone index.
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(SkeletonVertex), reinterpret_cast<void const*>(offsetof(SkeletonVertex, bone_id)));
	}

public:
	SkeletonMesh(Skeleton const& skeleton) {
		load_skeleton_(skeleton);
		create_gpu_buffers_();
		set_vertex_attributes_();
	}
	
	void draw_bones() {
		glBindVertexArray(vao_);
		glLineWidth(1.f);
		glDrawElements(GL_LINES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	}
	void draw_joints() {
		glBindVertexArray(vao_);
		glPointSize(3.f);
		glDrawElements(GL_POINTS, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	}
};

} // namespace testing

#endif
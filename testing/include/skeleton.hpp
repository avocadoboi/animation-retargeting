#ifndef ANIMATION_RETARGETING_TESTING_SKELETON_HPP
#define ANIMATION_RETARGETING_TESTING_SKELETON_HPP

#include "fbx.hpp"
#include "util.hpp"

#include "animation_retargeting.hpp"

#include <fmt/format.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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

	std::vector<T> extract_values() const
	{
		auto values = std::vector<T>{};
		values.reserve(keyframes_.size());

		for (auto const& keyframe : keyframes_) {
			values.push_back(keyframe.value);
		}
		return values;
	}
	void set_values(std::vector<T> const& values) {
		for (auto const i : util::indices(keyframes_)) {
			keyframes_[i].value = values[i];
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

	// The bone's index in the skeleton's bone array.
	Id id;

	// The bone's global bind transform and its inverse.
	glm::mat4 bind_transform;
	glm::mat4 inverse_bind_transform;

	// The bone's current global transform in an animation.
	glm::mat4 global_transform;

	// The transform applied to bones and skin in an animation, relative to the bind pose.
	// It is equal to inverse_bind_transform * global_transform.
	glm::mat4 animation_transform;

	// Transforms to apply between scale/rotation/translation components, to account for pivots etc.
	glm::mat4 pre_scaling;
	glm::mat4 pre_rotation;
	glm::mat4 pre_translation;
	glm::mat4 post_translation;

	// These are the components of the bone's local bind transform.
	glm::vec3 local_bind_scale;
	glm::quat local_bind_rotation;
	glm::vec3 local_bind_translation;

	// Keyframes for the scale/rotation/translation components of the bone's local transform.
	AnimationTrack<glm::vec3> scale_track;
	AnimationTrack<glm::quat> rotation_track;
	AnimationTrack<glm::vec3> translation_track;

	glm::mat4 calculate_local_transform(glm::vec3 const scale, glm::quat const rotation, glm::vec3 const translation) const
	{
		return post_translation * glm::translate(glm::mat4{1.f}, translation) * pre_translation * glm::mat4_cast(rotation) * pre_rotation * glm::scale(glm::mat4{1.f}, scale) * pre_scaling;
	}

	Bone() = default;
	Bone(Bone const* parent, std::string name, Id const id, FbxNode* const bone_node) :
		parent{parent},
		name{std::move(name)},
		id{id},
		bind_transform{util::fbx_to_glm(bone_node->EvaluateGlobalTransform())}
	{
		/*
			From the FBX SDK docs:
			World = ParentWorld * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1
			Here, I put Sp-1 into bone.pre_scaling, Rpost * Rp-1 * Soff * Sp into bone.pre_rotation, and Roff * Rp * Rpre into bone.pre_translation.
			Notice that me and the FBX SDK use different pre/post words... I think mine are correct because the right side of a matrix
			multiplication is the transform that is applied first. T * R * S means scaling happens first, then rotation, then translation.
		*/

		auto const fbx_scaling_pivot = util::fbx_to_glm(bone_node->GetScalingPivot(FbxNode::eSourcePivot));
		pre_scaling = glm::translate(glm::mat4{1.f}, -fbx_scaling_pivot);

		auto const fbx_rotation_pivot = util::fbx_to_glm(bone_node->GetRotationPivot(FbxNode::eSourcePivot));
		auto const fbx_post_rotation = util::euler_angles_to_mat4_xyz(glm::radians(util::fbx_to_glm(bone_node->GetPostRotation(FbxNode::eSourcePivot))));
		pre_rotation = fbx_post_rotation * glm::translate(glm::mat4{1.f}, -fbx_rotation_pivot + util::fbx_to_glm(bone_node->GetScalingOffset(FbxNode::eSourcePivot)) + fbx_scaling_pivot);

		auto const fbx_pre_rotation = util::euler_angles_to_mat4_xyz(glm::radians(util::fbx_to_glm(bone_node->GetPreRotation(FbxNode::eSourcePivot))));
		pre_translation = glm::translate(glm::mat4{1.f}, util::fbx_to_glm(bone_node->GetRotationOffset(FbxNode::eSourcePivot)) + fbx_rotation_pivot) * fbx_pre_rotation;

		// Add the parent node's global transform if this is a root bone.
		post_translation = parent ? glm::mat4{1.f} : util::fbx_to_glm(bone_node->GetParent()->EvaluateGlobalTransform());

		// bone.local_bind_scale = util::fbx_to_glm(bone_node->LclScaling.Get());
		// bone.local_bind_rotation = glm::quat{glm::radians(util::fbx_to_glm(bone_node->LclRotation.Get()))};
		// bone.local_bind_translation = util::fbx_to_glm(bone_node->LclTranslation.Get());

        // auto const local_transform = bone.calculate_local_transform(bone.local_bind_scale, bone.local_bind_rotation, bone.local_bind_translation);

		// bone.bind_transform = parent ? parent->bind_transform * local_transform : local_transform;
	}
};

class Skeleton {
private:
	using Bones_ = util::StaticVector<Bone, 256>;
	std::unique_ptr<Bones_> bones_ = std::make_unique<Bones_>();

	void add_bone_(FbxNode* const bone_node, Bone const* parent)
	{
		auto const name = util::trimmed_bone_name(bone_node);
		
		if (util::is_end_bone(name)) {
			return;
		}

		bones_->push_back(Bone{parent, name, static_cast<Bone::Id>(bones_->size()), bone_node});

		parent = &bones_->back();

		for (auto const i : util::indices(bone_node->GetChildCount())) {
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
		
		if (auto const* const attribute = node->GetNodeAttribute()) {
			if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
				add_bone_(node, nullptr);
				return;
			}
		}
		
		for (auto const i : util::indices(node->GetChildCount())) {
			load_from_fbx_node(node->GetChild(i));
		}		
	}

	void calculate_local_bind_components() 
	{
		for (auto& bone : *bones_) {
			bone.inverse_bind_transform = glm::inverse(bone.bind_transform);

			auto const local = bone.parent ? bone.parent->inverse_bind_transform * bone.bind_transform : glm::inverse(bone.post_translation) * bone.bind_transform;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(local, bone.local_bind_scale, bone.local_bind_rotation, bone.local_bind_translation, skew, perspective);
		}
	}

	auto extract_animation() const 
		-> animation_retargeting::Animation
	{
		auto animation = animation_retargeting::Animation{};
		animation.bones.reserve(bones_->size());
		
		for (auto const& bone : *bones_) {
			animation.bones.push_back(animation_retargeting::AnimatedBone{
				bone.scale_track.extract_values(),
				bone.rotation_track.extract_values(),
				bone.translation_track.extract_values()
			});
		}
		return animation;
	}
	auto extract_pose() const 
		-> animation_retargeting::Pose
	{
		auto pose = animation_retargeting::Pose{};
		pose.bones.reserve(bones_->size());
		
		for (auto const& bone : *bones_) {
			pose.bones.push_back(animation_retargeting::PoseBone{
				bone.name,
				bone.parent ? bone.parent->id : animation_retargeting::PoseBone::no_parent,
				bone.local_bind_scale,
				bone.local_bind_rotation,
				bone.local_bind_translation,
			});
		}
		
		return pose;
	}

	void set_animation_values(animation_retargeting::Animation const& animation) 
	{
		for (auto const i : util::indices(animation.bones))
		{
			auto& bone = (*bones_)[i];
			auto const& animation_bone = animation.bones[i];
			bone.scale_track.set_values(animation_bone.scales);
			bone.rotation_track.set_values(animation_bone.rotations);
			bone.translation_track.set_values(animation_bone.translations);
		}
	}

	void set_bind_pose(animation_retargeting::Pose const& pose)
	{
		for (auto const i : util::indices(pose.bones))
		{
			auto& bone = (*bones_)[i];
			bone.local_bind_scale = pose.bones[i].scale;
			bone.local_bind_rotation = pose.bones[i].rotation;
			bone.local_bind_translation = pose.bones[i].translation;
			
			auto const local = bone.calculate_local_transform(bone.local_bind_scale, bone.local_bind_rotation, bone.local_bind_translation);

			bone.bind_transform = bone.parent ? bone.parent->bind_transform * local : local;
			bone.inverse_bind_transform = glm::inverse(bone.bind_transform);
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

			vertices_.push_back(SkeletonVertex{bone.bind_transform * glm::vec4{0.f, 0.f, 0.f, 1.f}, bone.id});
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
	
	void draw_bones() const {
		glBindVertexArray(vao_);
		glLineWidth(1.f);
		glDrawElements(GL_LINES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	}
	void draw_joints() const {
		glBindVertexArray(vao_);
		glPointSize(3.f);
		glDrawElements(GL_POINTS, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	}
};

} // namespace testing

#endif
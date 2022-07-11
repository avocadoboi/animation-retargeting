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

	Skeleton& skeleton_;
	std::chrono::time_point<Clock_> start_time_{Clock_::now()};

	void load_animation_(FbxNode* const node, FbxAnimLayer* const animation_layer)
	{
		if (auto const* const attribute = node->GetNodeAttribute())
		{
			if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				if (auto* const bone = skeleton_.bone_by_name(util::trimmed_bone_name(node).c_str())) 
				{
					bone->scale_track = AnimationTrack<glm::vec3>{animation_layer, node->LclScaling};
					bone->rotation_track = AnimationTrack<glm::quat>{animation_layer, node->LclRotation};
					bone->translation_track = AnimationTrack<glm::vec3>{animation_layer, node->LclTranslation};
				}
			}
		}
		
		for (auto const i : util::indices(node->GetChildCount()))
		{
			load_animation_(node->GetChild(i), animation_layer);
		}	
	}

	void load_animations_(FbxScene const* const scene, FbxNode* const root_node) 
	{
		auto const stack_count = scene->GetSrcObjectCount(FbxCriteria::ObjectType(FbxAnimStack::ClassId));
		for (auto const stack_index : util::indices(stack_count))
		{
			auto const* const animation_stack = static_cast<FbxAnimStack const*>(scene->GetSrcObject(FbxCriteria::ObjectType(FbxAnimStack::ClassId), stack_index));

			auto const layer_count = animation_stack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId));
			for (auto const layer_index : util::indices(layer_count))
			{
				auto* const animation_layer = static_cast<FbxAnimLayer*>(animation_stack->GetMember(FbxCriteria::ObjectType(FbxAnimLayer::ClassId), layer_index));
				load_animation_(root_node, animation_layer);
			}
		}
	}

public:
	Animation(char const* const fbx_path, Skeleton& skeleton) :
		skeleton_{skeleton}
	{
		if (!fbx_path || *fbx_path == char{}) {
			return;
		}
		
		auto manager = fbx::create<FbxManager>();
		
		auto settings = create_import_settings_(manager.get());
		manager->SetIOSettings(settings.get());
		
		auto scene = fbx::import_scene(manager.get(), fbx_path);

		FbxAxisSystem::OpenGL.DeepConvertScene(scene.get());

		auto* const root_node = scene->GetRootNode();

		if (!root_node) {
			throw std::runtime_error{"An FBX scene did not contain a root node."};
		}

		load_animations_(scene.get(), root_node);		
	}

	void restart() {
		start_time_ = Clock_::now();
	}

	void update_bone_matrices()
	{
		auto const time = std::chrono::duration_cast<Seconds>(Clock_::now() - start_time_);
		// auto const time = Seconds{};

		// Hack, remove later
		if (time > skeleton_.bones()[0].translation_track.duration()) {
			restart();
		}

		for (auto& bone : skeleton_.bones())
		{
			auto const local_scale = bone.scale_track.evaluate(time, bone.local_bind_scale);
			auto const local_rotation = bone.rotation_track.evaluate(time, bone.local_bind_rotation);
			auto const local_translation = bone.translation_track.evaluate(time, bone.local_bind_translation);
			// auto const local_scale = bone.local_bind_scale;
			// auto const local_rotation = bone.local_bind_rotation;
			// auto const local_translation = bone.local_bind_translation;

			auto const local_transform = bone.calculate_local_transform(local_scale, local_rotation, local_translation);
			
			bone.global_transform = bone.parent ? bone.parent->global_transform * local_transform : local_transform;

			bone.animation_transform = bone.global_transform * bone.inverse_bind_transform;
		}
	}
};

} // namespace testing

#endif
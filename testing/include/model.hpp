#ifndef ANIMATION_RETARGETING_TESTING_MODEL_HPP
#define ANIMATION_RETARGETING_TESTING_MODEL_HPP

#include "fbx.hpp"
#include "skeleton.hpp"
#include "texture.hpp"

#include <fmt/format.h>

namespace testing {

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture_coordinates;

	static constexpr auto max_bone_influence = std::size_t{4};
	std::array<Bone::Id, max_bone_influence> bone_ids{};
	std::array<float, max_bone_influence> bone_weights{};

	void add_bone(Bone::Id const id, float const weight)
	{
		if (weight < 0.02f) {
			return;
		}
		for (auto const i : util::indices(max_bone_influence)) {
			if (bone_weights[i] == 0.f) {
				bone_ids[i] = id;
				bone_weights[i] = weight;
				return;
			}
		}
		fmt::print("Too many bone influences!!! Tried to add weight {}\n", weight);
	}
};

class Mesh {
private:
	GLsizei index_count_;
	GLuint texture_id_;

	GLuint vao_;
	GLuint vbo_;
	GLuint ebo_;

	void create_gpu_buffers_(std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices)
	{        
		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);

		// Vertices.
		glGenBuffers(1, &vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, util::vector_byte_size(vertices), vertices.data(), GL_STATIC_DRAW);

		// Vertex indices.
		glGenBuffers(1, &ebo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, util::vector_byte_size(indices), indices.data(), GL_STATIC_DRAW);
	}

	void set_vertex_attributes_()
	{
		// Vertex positions.
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

		// Vertex normals.
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, normal)));

		// Vertex texture coordinates.
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, texture_coordinates)));

		// Influencing bone IDs.
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, Vertex::max_bone_influence, GL_UNSIGNED_INT, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, bone_ids)));
		static_assert(std::is_same<Bone::Id, GLuint>::value);

		// Influencing bone weights.
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, Vertex::max_bone_influence, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, bone_weights)));
	}

public:
	Mesh(std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices, GLuint const texture_id) :
		index_count_{static_cast<GLsizei>(indices.size())}, texture_id_{texture_id}
	{
		create_gpu_buffers_(vertices, indices);
		set_vertex_attributes_();
	}

	void draw() const 
	{
		if (texture_id_) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_id_);
		}

		glBindVertexArray(vao_);
		glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
	}
};

class Model {
private:
	std::vector<Mesh> meshes_;
	Texture texture_;
	Skeleton skeleton_;

	void set_bone_bind_transform_from_cluster(Bone::Id const bone_id, FbxCluster const* const cluster) 
	{
		auto* const bone = skeleton_.bone_by_id(bone_id);

		FbxAMatrix bind_matrix;
		cluster->GetTransformLinkMatrix(bind_matrix);

		bone->bind_transform = util::fbx_to_glm(bind_matrix);
	}

	void connect_bones_to_vertices_(FbxMesh const* const mesh, std::vector<Vertex>& vertices)
	{
		auto const* const skin = static_cast<FbxSkin const*>(mesh->GetDeformer(0, FbxDeformer::eSkin));

		auto const cluster_count = skin->GetClusterCount();
		for (auto const cluster_index : util::indices(cluster_count))
		{
			auto const* const cluster = skin->GetCluster(cluster_index);

			auto const name = util::trimmed_bone_name(cluster->GetLink());

			if (util::is_end_bone(name)) {
				continue;
			}

			auto const bone_id = skeleton_.bone_id_by_name(name.c_str());

			set_bone_bind_transform_from_cluster(bone_id, cluster);

			auto const control_point_count = cluster->GetControlPointIndicesCount();
			auto const* const control_point_indices = cluster->GetControlPointIndices();
			auto const* const control_point_weights = cluster->GetControlPointWeights();

			for (auto const j : util::indices(control_point_count))
			{
				vertices[control_point_indices[j]].add_bone(bone_id, static_cast<float>(control_point_weights[j]));
			}
		}
	}

	void load_mesh_(FbxMesh const* const mesh)
	{
		auto const transform = util::fbx_to_glm(mesh->GetNode()->EvaluateGlobalTransform());
		
		auto const* const vertices_source = mesh->GetControlPoints();

		auto const* const normal_layer = mesh->GetElementNormal();
		auto const* const uv_layer = mesh->GetElementUV();
		
		auto vertices = std::vector<Vertex>(mesh->GetControlPointsCount());

		for (auto const i : util::indices(vertices))
		{
			vertices[i].position = transform*glm::vec4{fbx::to_glm_vec(vertices_source[i]), 1.f};

			if (normal_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByControlPoint)
			{
				vertices[i].normal = fbx::layer_element_at(normal_layer, static_cast<int>(i));
			}
			
			if (uv_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByControlPoint)
			{
				vertices[i].texture_coordinates = fbx::layer_element_at(uv_layer, static_cast<int>(i));
			}
		}

		auto const* const indices_source = mesh->GetPolygonVertices();
		auto indices = std::vector<GLuint>(mesh->GetPolygonVertexCount());

		for (auto const i : util::indices(indices))
		{
			indices[i] = static_cast<GLuint>(indices_source[i]);

			if (normal_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByPolygonVertex)
			{
				vertices[indices[i]].normal = fbx::layer_element_at(normal_layer, static_cast<int>(i));
			}
			
			if (uv_layer->GetMappingMode() == FbxLayerElement::EMappingMode::eByPolygonVertex)
			{
				vertices[indices[i]].texture_coordinates = fbx::layer_element_at(uv_layer, static_cast<int>(i));
			}
		}

		connect_bones_to_vertices_(mesh, vertices);
		
		meshes_.emplace_back(vertices, indices, texture_.id());
	}

	void process_node_(FbxNode const* const node)
	{
		if (auto const* const attribute = node->GetNodeAttribute()) {
			if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh) {
				load_mesh_(static_cast<FbxMesh const*>(attribute));
			}
		}

		for (auto const i : util::indices(node->GetChildCount())) {
			process_node_(node->GetChild(i));
		}
	}

	static fbx::Unique<FbxIOSettings> create_import_settings_(FbxManager* const manager)
	{
		auto settings = fbx::create<FbxIOSettings>(manager, IOSROOT);
		settings->SetBoolProp(IMP_FBX_ANIMATION, false);
		settings->SetBoolProp(IMP_FBX_AUDIO, false);
		settings->SetBoolProp(IMP_FBX_BINORMAL, false);
		settings->SetBoolProp(IMP_FBX_CONSTRAINT, false);
		settings->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
		settings->SetBoolProp(IMP_FBX_MATERIAL, false);
		settings->SetBoolProp(IMP_FBX_TANGENT, false);
		settings->SetBoolProp(IMP_FBX_TEXTURE, false);
		settings->SetBoolProp(IMP_FBX_VERTEXCOLOR, false);
		return settings;
	}

public:
	Model() = default;

	Model(char const* const fbx_path, Texture texture) :
		texture_{std::move(texture)}
	{
		auto manager = fbx::create<FbxManager>();
		
		auto settings = create_import_settings_(manager.get());
		manager->SetIOSettings(settings.get());
		
		auto scene = fbx::import_scene(manager.get(), fbx_path);

		FbxAxisSystem::OpenGL.DeepConvertScene(scene.get());

		FbxGeometryConverter{manager.get()}.Triangulate(scene.get(), true);

		auto* const root_node = scene->GetRootNode();

		if (!root_node) {
			throw std::runtime_error{"An FBX scene did not contain a root node."};
		}

		skeleton_.load_from_fbx_node(root_node);

		for (auto const i : util::indices(root_node->GetChildCount())) {
			process_node_(root_node->GetChild(i));
		}

		skeleton_.calculate_local_bind_components();
	}

	Skeleton const& skeleton() const {
		return skeleton_;
	}
	
	Skeleton& skeleton() {
		return skeleton_;
	}
	
	void draw() const {
		for (auto const& mesh : meshes_) {
			mesh.draw();
		}
	}
};

} // namespace testing

#endif
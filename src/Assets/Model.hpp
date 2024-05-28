#pragma once

#include "Material.hpp"
#include "Procedural.hpp"
#include "Vertex.hpp"
#include "Texture.hpp"
#include "UniformBuffer.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Assets
{
	class Model final
	{
	public:
		
		static void FlattenVertices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
		static Model LoadModel(const std::string& filename, std::vector<Texture> &textures);
		static Model CreateCornellBox(const float scale);
		static Model CreateBox(const glm::vec3& p0, const glm::vec3& p1, const Material& material);
		static Model CreateSphere(const glm::vec3& center, float radius, const Material& material, bool isProcedural);
		static Model CreateQuad(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& dir, const Material& material);
		static void LoadGLTFScene(const std::string& filename, std::vector<class Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures);
		
		Model& operator = (const Model&) = delete;
		Model& operator = (Model&&) = delete;

		Model() = default;
		Model(const Model&) = default;
		Model(Model&&) = default;
		~Model() = default;

		void SetMaterial(const Material& material);
		
		const std::vector<Vertex>& Vertices() const { return vertices_; }
		const std::vector<uint32_t>& Indices() const { return indices_; }
		const std::vector<Material>& Materials() const { return materials_; }
		const std::vector<LightObject>& Lights() const { return lights_; }
		

		const class Procedural* Procedural() const { return procedural_.get(); }

		uint32_t NumberOfVertices() const { return static_cast<uint32_t>(vertices_.size()); }
		uint32_t NumberOfIndices() const { return static_cast<uint32_t>(indices_.size()); }
		uint32_t NumberOfMaterials() const { return static_cast<uint32_t>(materials_.size()); }
	private:

		Model(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<Material>&& materials, std::vector<LightObject>&& lights, const class Procedural* procedural);

		std::vector<Vertex> vertices_;
		std::vector<uint32_t> indices_;
		std::vector<Material> materials_;
		std::shared_ptr<const class Procedural> procedural_;

		std::vector<LightObject> lights_;
	};

	class Node final
	{
	public:

		static Node CreateNode(glm::mat4 transform, int id, bool procedural);
		Node& operator = (const Node&) = delete;
		Node& operator = (Node&&) = delete;

		Node() = default;
		Node(const Node&) = default;
		Node(Node&&) = default;
		~Node() = default;
		
		void Transform(const glm::mat4& transform) {transform_ = transform;}
		const glm::mat4& const WorldTransform() const {return transform_; }
		int GetModel() const {return modelId_;}
		bool IsProcedural() const {return procedural_;}
	private:

		Node(glm::mat4 transform, int id, bool procedural);
		
		glm::mat4 transform_;
		int modelId_;
		bool procedural_;
	};

	
}

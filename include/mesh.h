#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "ray.h"
#include "triangle.h"

namespace rtt
{
	class MeshValues
	{
	public:
		std::vector<glm::vec3> vertices_;
		std::vector<glm::vec3> normals_;
		std::vector<glm::vec2> uvs_;

		MeshValues(const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &uvs)
		: vertices_(std::move(vertices)), normals_(std::move(normals)), uvs_(std::move(uvs)) {}
	};



	template <class IndexSize = unsigned int>
	class MeshIndices 
	{
	public:
		std::vector<TriangleIndices<IndexSize>> triangles;
		MeshIndices() = default;
		MeshIndices(const std::vector<IndexSize> vertex_indices, const std::vector<IndexSize> normal_indices, const std::vector<IndexSize> uv_indices) 
		{
			for (unsigned int i = 0; i < vertex_indices.size() / 3; i++) {				
				triangles.emplace_back(vertex_indices[3 * i], vertex_indices[3 * i + 1], vertex_indices[3 * i + 2],
					normal_indices[3 * i], normal_indices[3 * i + 1], normal_indices[3 * i + 2],
					uv_indices[3 * i], uv_indices[3 * i + 1], uv_indices[3 * i + 2]);
			}
		}
		MeshIndices(const std::vector<TriangleIndices<IndexSize>> triangles) : triangles(std::move(triangles)) {} 		
	};

	class Mesh
	{
	public:
		std::vector<Triangle> triangles;
		Mesh() = default;
		
		template<class IndexSize>
		Mesh(const std::vector<glm::vec3> &mesh_vertices, const MeshIndices<IndexSize>& vertex_indices) 
		{
			for (const auto& tri: vertex_indices.triangles)
				triangles.push_back(Triangle(mesh_vertices, tri));
		}

		void intersect(const Ray &ray, Intersection &its) const 
		{ 
			for (const auto & tri: triangles) tri.intersect(ray, its);
		}	
	};
}

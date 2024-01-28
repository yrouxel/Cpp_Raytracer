#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp> // to_string mat4

namespace rtt
{	
	class Intersection 
	{
	public: 
		bool intersection;
		float distance;
		glm::vec3 normal;
		glm::vec3 position;
		// we might need the uv
		Intersection() : distance(std::numeric_limits<float>::max()), intersection(false), normal(glm::vec3(0.f)), position(glm::vec3(0.f)) {}
	};

	template<class IndexSize, class MaterialIndexSize = unsigned short>
	class TriangleIndices
	{
	public:
		IndexSize index_vertices[3]; //3f<3f>
		// IndexSize index_normals[3]; //3f<3f>
		// IndexSize index_uvs[3]; //3f<2f>
		// MaterialIndexSize material_index;

		TriangleIndices() = default;
		TriangleIndices(const IndexSize &vert0, const IndexSize &vert1, const IndexSize &vert2,
			 	 const IndexSize &norm0, const IndexSize &norm1, const IndexSize &norm2,
				 const IndexSize &uv0,   const IndexSize &uv1,   const IndexSize &uv2)
				//  const MaterialIndexSize material_index) 
		{
			index_vertices[0] = vert0;
			index_vertices[1] = vert1;
			index_vertices[2] = vert2;
			// index_normals[0]  = norm0;
			// index_normals[1]  = norm1;
			// index_normals[2]  = norm2;
			// index_uvs[0]      = uv0;
			// index_uvs[1]      = uv1;
			// index_uvs[2]      = uv2;
			// material_index    = material_index;
		}
	};

	class Triangle
	{
	public:
		glm::vec3 e1, e2, v0;

		Triangle() = default;

		template<class IndexSize>
		Triangle(const std::vector<glm::vec3> &mesh_vertices, const TriangleIndices<IndexSize>& triangle)
		{ update(mesh_vertices, triangle); }
		
		template<class IndexSize>
		void update(const std::vector<glm::vec3> &mesh_vertices, const TriangleIndices<IndexSize>& triangle)
		{
			e1 = mesh_vertices[triangle.index_vertices[1]] - mesh_vertices[triangle.index_vertices[0]]; 
			e2 = mesh_vertices[triangle.index_vertices[2]] - mesh_vertices[triangle.index_vertices[0]]; 
			v0 = mesh_vertices[triangle.index_vertices[0]];
		}

		void intersect(const Ray &ray, Intersection &its) const {
			// Strahl-Dreieck-Schitt aus CG Vorlesung
			// e1 = v1 - v0, e2 = v2 - v0
			// o + t * d = v0 + l2 * e1 + l3 * e2
			// o - v0 = [t l2 l3]  [d e1 e2] -> solve the linear system by Cramer's rule

			const glm::vec3 pvec = glm::cross(ray.direction, e2);
    		const float det = glm::dot(e1, pvec);
			if(det <= 0.f) return; // if determinant is negative we have a backface, 0= parallel
			const float inv_det = 1.f / det;

			const glm::vec3 tvec = ray.origin - v0;
			const float l2 = glm::dot(tvec, pvec) * inv_det;;
			if (l2 < 0.f || l2 >= 1.f) return;			

			const glm::vec3 qvec = glm::cross(tvec, e1);
			const float l3 = glm::dot(ray.direction, qvec) * inv_det;
			if (l3 < 0.f || l3 + l2 > 1.f) return;

			const float t = glm::dot(e2, qvec) * inv_det;
			if (t <= 0.f || t >= its.distance) return;
			
			its.distance = t;
			its.normal = glm::normalize(cross(e1,e2));
            its.position = ray.origin + ray.direction * t;
			its.intersection = true;
		}
	};
}

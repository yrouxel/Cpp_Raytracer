#pragma once

#include <vector>
#include <algorithm>

#include "bounding_volume.h"
#include "triangle.h"
#include "mesh.h"

#include "parser.h"

#include "glm/glm.hpp"

namespace rtt
{
	struct CompAxis
	{
		unsigned short axis;
		CompAxis(unsigned short axis) : axis(axis) {}
		bool operator()(std::pair<glm::vec3, unsigned long> v1, std::pair<glm::vec3, unsigned long> v2)
		{ 
			if (v1.first[axis] < v2.first[axis]) return true;
			else if (v1.first[axis] > v2.first[axis]) return false;
			else if (v1.first[(axis+1)%3] < v2.first[(axis+1)%3]) return true;
			else if (v1.first[(axis+1)%3] > v2.first[(axis+1)%3]) return false;
			else if (v1.first[(axis+2)%3] < v2.first[(axis+2)%3]) return true;
			else return false;
		}
	};


	class BVH
	{
	public:
		glm::vec3 min_, max_;
		virtual bool intersect(Intersection &its, const Ray &ray) const = 0;
		virtual ~BVH() = default;
	};

	template<class IndexSize, unsigned int TrianglesPerLeaf = 12> //empirically the best value
	class BVH_template : public BVH
	{
	private:
		std::vector<BoundingVolume> volumes; //binary heap
		std::vector<Mesh> triangles_values;
		MeshValues vertices;
		unsigned int number_of_leaves;

	public:
		BVH_template(const MeshValues& vertices, const MeshIndices<IndexSize> &triangles, const std::vector<glm::vec3> &ordered_vertices)
			: vertices(std::move(vertices))
		{
			std::cout << "number of triangles: " << triangles.triangles.size() << "\n";
			
			number_of_leaves = findNextPowerOfTwo(triangles.triangles.size());
			volumes = std::vector<BoundingVolume>(number_of_leaves*2);
			triangles_values = std::vector<Mesh>(number_of_leaves);

			std::cout << "number of leaves " << number_of_leaves << "\n";

			std::vector<std::pair<glm::vec3, unsigned long>> grav_centers;
			grav_centers.reserve(triangles.triangles.size());
			//averaging isn't actually useful since we want a global order of the triangles
			for (unsigned long i = 0; i < ordered_vertices.size(); i+=3)
				grav_centers.emplace_back(std::make_pair(ordered_vertices[i] + ordered_vertices[i+1] + ordered_vertices[i+2], i/3));
			recursively_split(grav_centers, triangles.triangles, 0);
			min_ = volumes[0].bounds[0];
			max_ = volumes[0].bounds[1];
		}

		bool intersect(Intersection &its, const Ray &ray) const
		{
			float temp;
			const Ray inv_ray(ray.origin, 1.f / ray.direction);
			const Ray inv_ray45(glm::rotate(ray.origin, glm::radians(45.f), glm::vec3(1, 1, 1)), 1.f / glm::rotate(ray.direction, glm::radians(45.f), glm::vec3(1, 1, 1)));
			if (volumes[0].intersect_box(inv_ray, inv_ray45, temp)) 
				return intersect_recursively(its, ray, inv_ray, inv_ray45, 0);
			return false;
		}

	private:
		void recursively_split(std::vector<std::pair<glm::vec3, unsigned long>> &grav_centers, const std::vector<TriangleIndices<IndexSize>> &triangles, const unsigned int index) 
		{
			std::vector<float> diff;
			for (unsigned short i = 0; i < 3; i++)
			{
    			const auto [min, max] = std::minmax_element(grav_centers.begin(), grav_centers.end(), CompAxis(i));
				diff.push_back(max->first[i] - min->first[i]);
			}
			unsigned short axis = std::distance(diff.begin(), std::max_element(diff.begin(), diff.end()));
			std::nth_element(grav_centers.begin(), grav_centers.begin()+grav_centers.size()/2, grav_centers.end(), CompAxis(axis));
			if (grav_centers.size() < 2*TrianglesPerLeaf)//edge case, we want 2 leaves every time
			{
				std::vector<TriangleIndices<IndexSize>> leaf_triangles;
				leaf_triangles.reserve(grav_centers.size());
				for (const auto &[key, value] : grav_centers) 
					leaf_triangles.push_back(triangles[value]);

				// std::vector<Triangle<IndexSize>> leaf_triangles(grav_centers.size());
				// for (unsigned int i = 0; i < grav_centers.size(); i++) 
				// 	leaf_triangles[i] = triangles[grav_centers[i].second];
				std::vector<TriangleIndices<IndexSize>> p1(leaf_triangles.begin(),  leaf_triangles.begin() + leaf_triangles.size()/2);
				std::vector<TriangleIndices<IndexSize>> p2(leaf_triangles.begin() + leaf_triangles.size()/2, leaf_triangles.end());
				MeshIndices<IndexSize> m1(p1);
				MeshIndices<IndexSize> m2(p2);
				Mesh mr1(vertices.vertices_, m1);
				Mesh mr2(vertices.vertices_, m2);
				triangles_values[index*2+1 - (number_of_leaves-1)] = mr1;
				triangles_values[index*2+2 - (number_of_leaves-1)] = mr2;
				volumes[index*2+1] = BoundingVolume(vertices.vertices_, m1);
				volumes[index*2+2] = BoundingVolume(vertices.vertices_, m2);
			}
			else
			{
				std::vector<std::pair<glm::vec3, unsigned long>> p1(grav_centers.begin(), grav_centers.begin() + grav_centers.size()/2);
				std::vector<std::pair<glm::vec3, unsigned long>> p2(grav_centers.begin() + grav_centers.size()/2, grav_centers.end());
				recursively_split(p1, triangles, index*2+1);
				recursively_split(p2, triangles, index*2+2);
			}
			volumes[index] = BoundingVolume(volumes[index*2+1], volumes[index*2+2]);
		}

		bool intersect_recursively(Intersection &its, const Ray &ray, const Ray &inv_ray, const Ray& inv_ray45, unsigned int index) const
		{
			if (index >= number_of_leaves-1)
			{
				triangles_values[index - (number_of_leaves-1)].intersect(ray, its);
				return its.intersection;
			}

			bool box[2];
			float temp[2]; 
			box[0] = volumes[index*2+1].intersect_box(inv_ray, inv_ray45, temp[0]);
			box[1] = volumes[index*2+2].intersect_box(inv_ray, inv_ray45, temp[1]);
			temp[0] = box[0] ? temp[0] : std::numeric_limits<float>::max();
			temp[1] = box[1] ? temp[1] : std::numeric_limits<float>::max();

			if (box[temp[0] >= temp[1]] && intersect_recursively(its, ray, inv_ray, inv_ray45, index*2+1 + (temp[0] >= temp[1])) && its.distance < std::max(temp[0], temp[1])) return true;
			if (box[1 - (temp[0] >= temp[1])]) intersect_recursively(its, ray, inv_ray, inv_ray45, index*2+2 - (temp[0] >= temp[1]));
			return its.intersection;
		}


		static unsigned int findNextPowerOfTwo(unsigned int value)
		{
			value = (value + TrianglesPerLeaf - 1) / TrianglesPerLeaf;

			unsigned int next = 1;
			while (next < value)
				next *= 2;
			return next;
		}
	};

	template<class IndexSize>
	const std::unique_ptr<BVH> create_template_BVH(vector<vec3> &vertices, vector<vec2> &uvs, vector< vec3> &normals,
		vector<unsigned long> &vertex_indices, vector<unsigned long> &uv_indices, vector<unsigned long> &normal_indices, 
		vector<vec3> &ordered_vertices)
	{
			vector<IndexSize> new_vertex_indices;
			vector<IndexSize> new_uv_indices;
			vector<IndexSize> new_normal_indices;

			new_vertex_indices.reserve(vertex_indices.size());
			new_uv_indices.reserve(vertex_indices.size());
			new_normal_indices.reserve(vertex_indices.size());

			for (unsigned long i = 0; i < vertex_indices.size(); i++)
				new_vertex_indices.push_back(vertex_indices[i]);
			for (unsigned long i = 0; i < vertex_indices.size(); i++)
				new_uv_indices.push_back(uv_indices[i]);	
			for (unsigned long i = 0; i < vertex_indices.size(); i++)
				new_normal_indices.push_back(normal_indices[i]);

			return std::make_unique<BVH_template<IndexSize>>(rtt::MeshValues(vertices, normals, uvs)
				, rtt::MeshIndices<IndexSize>(new_vertex_indices, new_normal_indices, new_uv_indices)
				, ordered_vertices);
	}

	std::unique_ptr<BVH> createBVH(const std::string dir_path, const std::string& scene_name)
	{
		vector<vec3> out_vertices;
		vector<vec2> out_uvs;
		vector<vec3> out_normals;
		vector<unsigned long> vertex_indices;
		vector<unsigned long> uv_indices;
		vector<unsigned long> normal_indices;
		vector<vec3> ordered_vertices;

		parseFile(dir_path, scene_name, out_vertices, out_uvs, out_normals, vertex_indices, uv_indices, normal_indices, ordered_vertices);

		if (out_vertices.size() <= std::numeric_limits<unsigned short>::max())
			return create_template_BVH<unsigned short>(out_vertices, out_uvs, out_normals, vertex_indices, uv_indices, normal_indices, ordered_vertices);
		if (out_vertices.size() <= std::numeric_limits<unsigned int>::max())
			return create_template_BVH<unsigned int>(out_vertices, out_uvs, out_normals, vertex_indices, uv_indices, normal_indices, ordered_vertices);
		return std::make_unique<BVH_template<unsigned long>>(rtt::MeshValues(out_vertices, out_normals, out_uvs)
			, rtt::MeshIndices<unsigned long>(vertex_indices, normal_indices, uv_indices)
			, ordered_vertices);
	}
}

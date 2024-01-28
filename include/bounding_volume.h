#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "ray.h"
#include "triangle.h"
#include "mesh.h"

namespace rtt
{
	//axis aligned bounding box
	class BoundingVolume
	{
	public:
		glm::vec3 bounds[2];
		glm::vec3 bounds45[2];
		BoundingVolume() = default;

		template<class IndexSize>
		BoundingVolume(const std::vector<glm::vec3> &vertices, MeshIndices<IndexSize> &mesh)
		{
			bounds[0] = glm::vec3(std::numeric_limits<float>::max());
			bounds[1] = glm::vec3(std::numeric_limits<float>::min());
			bounds45[0] = glm::vec3(std::numeric_limits<float>::max());
			bounds45[1] = glm::vec3(std::numeric_limits<float>::min());
			for (const auto & tri : mesh.triangles)
			{
				for (const auto index : tri.index_vertices)
				{
					const glm::vec3 rotated = glm::rotate(vertices[index], glm::radians(45.f), glm::vec3(1, 1, 1));
					for (unsigned short j = 0; j < 3; j++)
					{			
						bounds[0][j]   = std::min(bounds[0][j], vertices[index][j]);
						bounds[1][j]   = std::max(bounds[1][j], vertices[index][j]);
						bounds45[0][j] = std::min(bounds45[0][j], rotated[j]);
						bounds45[1][j] = std::max(bounds45[1][j], rotated[j]);
					}
				}
			}
		}

		BoundingVolume(const BoundingVolume &child1, const BoundingVolume &child2)
		{
			for (unsigned short j = 0; j < 3; j++)
			{			
				bounds[0][j]   = std::min(child1.bounds[0][j], child2.bounds[0][j]);
				bounds[1][j]   = std::max(child1.bounds[1][j], child2.bounds[1][j]);
				bounds45[0][j] = std::min(child1.bounds45[0][j], child2.bounds45[0][j]);
				bounds45[1][j] = std::max(child1.bounds45[1][j], child2.bounds45[1][j]);
			}
		}

        bool intersect_box(const Ray &inv_ray, const Ray &inv_ray45, float &min) const
        { 
            min = std::numeric_limits<float>::min();
            float max = std::numeric_limits<float>::max();
            return intersect_box_bounds(inv_ray, min, max, bounds) && intersect_box_bounds(inv_ray45, min, max, bounds45);
        }

    private:
        //explanation here : https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
        bool intersect_box_bounds(const Ray &inv_ray, float &min, float&max, const glm::vec3 bounds[2]) const
        {           
            const glm::vec3 t_1 = (bounds[0] - inv_ray.origin) * inv_ray.direction;
            const glm::vec3 t_2 = (bounds[1] - inv_ray.origin) * inv_ray.direction;

            const glm::vec3 t_min2 = glm::min(t_1, t_2);
            const glm::vec3 t_max2 = glm::max(t_1, t_2);

            min = std::max(min, std::max(std::max(t_min2.x, t_min2.y), t_min2.z));
            max = std::min(max, std::min(std::min(t_max2.x, t_max2.y), t_max2.z));

            return max > 0 && min <= max;
        }
	};
}
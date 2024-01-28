#pragma once

#include <glm/glm.hpp>

namespace rtt
{
	class Ray {
	public:
		float epsilon;
		glm::vec3 direction;
		glm::vec3 origin;
		Ray(const glm::vec3 &o, const glm::vec3 &dir) : epsilon(0.0001f), direction(dir), origin(o + epsilon * direction) {}
	};
}

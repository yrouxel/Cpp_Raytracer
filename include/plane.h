#pragma once

class Plane { // local coordinate system
	public:
		glm::vec3 z;
		glm::vec3 y;
		glm::vec3 x;
		Plane(glm::vec3 n) : z(n) {
			if (n.z != 0.f) {
				y = glm::normalize(glm::vec3{1.f, 1.f, (-n.x - n.y) / n.z});
			} else if (n.y != 0.f) {
				y = glm::normalize(glm::vec3{1.f, (-n.x -n.z) / n.y, 1.f});
			} else {
				y = glm::normalize(glm::vec3{(-n.y -n.z) / n.x, 1.f, 1.f});
			}
			x = glm::normalize(glm::cross(y, z));
		}

		glm::vec3 toGlobal(const glm::vec3 v) const {
			glm::mat4 base(glm::vec4(x, 0.f), glm::vec4(y, 0.f), glm::vec4(z, 0.f), glm::vec4{0.f, 0.f, 0.f, 1.f});
			return glm::vec3(base * glm::vec4(v, 0.f));
		}

		glm::vec3 toLocal(const glm::vec3 v) const {
			glm::mat4 base(glm::vec4(x.x, y.x, z.x, 0.f), glm::vec4(x.y, y.y, z.y, 0.f), glm::vec4(x.z, y.z, z.z, 0.f), glm::vec4{0.f, 0.f, 0.f, 1.f});
			return glm::vec3(base * glm::vec4(v, 0.f));
		}

		float cosLocal(const glm::vec3 v) const {
			return v.z; // with normal (0,0,1)
		}
};

#pragma once

#include <glm/glm.hpp>

#include "gui.h"

namespace rtt
{
	class Camera {
		private: // Camera set up
			float fov;
			float screenWidth;
			float screenHeight;
			glm::vec3 w;
			glm::vec3 u; 
			glm::vec3 v;

		public:
			float screenWidthDiv;
			float screenHeightDiv;
			float pixelX;
			float pixelY;

		public:
			glm::vec3 origin;
			glm::vec3 angle;
			
			Camera(const unsigned int width, const unsigned int height, const float angleFOV, const glm::vec3 origin, const glm::vec3 angle) : fov(angleFOV), origin(origin), angle(angle) {
				float aspectRatio = width / float(height); // Aspektratio [4:3, 16:9 ...]

				glm::vec3 up {0.f, 1.f, 0.f};
				glm::vec3 distanceVec {0.f, 0.f, -1.f}; // Camera and screen have always the same distance

				float h = tan(fov / 2.f); // Condition: centered camera // assume distance to screen is 1
				screenHeight = 2.f * h;
				screenWidth = screenHeight * aspectRatio;
				screenHeightDiv = h;
				screenWidthDiv = -h * aspectRatio ;

				// u,v, w Kamerakoord system
				w = glm::normalize(distanceVec);
				u = glm::normalize(cross(up, w)); 
				v = glm::normalize(cross(w, u));

				// Rotation matrix with Euler angle
				glm::mat4 rx = glm::rotate(glm::mat4(1.0f), angle.x, glm::vec3(1.0, 0.0, 0.0));
				glm::mat4 ry = glm::rotate(glm::mat4(1.0f), angle.y, glm::vec3(0.0, 1.0, 0.0));
				glm::mat4 rz = glm::rotate(glm::mat4(1.0f), angle.z, glm::vec3(0.0, 0.0, 1.0));

				w = glm::normalize(glm::vec3(rz * ry * rx * glm::vec4(w, 1.f)));
				u = glm::normalize(glm::vec3(rz * ry * rx * glm::vec4(u, 1.f)));
				v = glm::normalize(glm::vec3(rz * ry * rx * glm::vec4(v, 1.f)));

				pixelX = screenWidth  / (float) width;
				pixelY = screenHeight / (float) height;
			}

			glm::vec3 computeDirection(float uu, float vv) const
			{ 
				glm::vec3 dir = uu * u + vv * v - 1.f * w; // dir is in WKS ---> would be [uu, vv, -1] in KKS
				return glm::normalize(dir);
			}
	};
}

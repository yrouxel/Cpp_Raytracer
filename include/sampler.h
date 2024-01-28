#pragma once

#include <iostream>
#include <random> // The header for the generators.
#include <ctime> // To seed the generator.

class Sampler {
	public: 
		const unsigned int seed;
		std::mt19937_64 rng;
		std::uniform_real_distribution<float> uniform; // in range [0,1)
		Sampler() : seed(time(0)), rng(seed) {}
		float next1D() {
			return uniform(rng);
		}
		glm::vec2 next2D() {
			return glm::vec2(uniform(rng), uniform(rng) );
		}
};
// we pretend this is in NKS, i.e. normal is [0, 0, 1]
const glm::vec3 sampleHemisphere(Sampler &sampler) {
	glm::vec2 randomNumber = sampler.next2D();
	const float xhi1 = randomNumber[0];
	const float xhi2 = randomNumber[1];

	const float phi = xhi1 * 2.f * M_PI;
	const float cosTheta = 1.f - xhi2;
	const float theta = std::acos(cosTheta);

	const float x = std::sin(theta) * std::cos(phi);
	const float y = std::sin(theta) * std::sin(phi);
	const float z = cosTheta;

	return glm::vec3(x,y,z);

}

const glm::vec3 sampleSphere(glm::vec2 randomNumber) {
	//glm::vec2 randomNumber = sampler.next2D();
	const float xhi1 = randomNumber[0];
	const float xhi2 = randomNumber[1];

	const float phi = xhi1 * 2.f * M_PI;
	const float cosTheta = 1.f - 2.f * xhi2;
	const float sinTheta = std::sqrt(1.f - cosTheta * cosTheta);

	const float x = sinTheta * std::cos(phi);
	const float y = sinTheta * std::sin(phi);
	const float z = cosTheta;

	return glm::vec3(x,y,z);

}

const glm::vec3 sampleGGXnormalD(Sampler &sampler, float alpha) {
	glm::vec2 randomNumber = sampler.next2D();
	const float xhi1 = randomNumber[0];
	const float xhi2 = randomNumber[1];

	const float phi = xhi2 * 2.f * M_PI;
	const float tanTheta = alpha * std::sqrt(xhi1) / std::sqrt(1.f - xhi1);
	const float theta = std::atan(tanTheta);

	const float x = std::sin(theta) * std::cos(phi);
	const float y = std::sin(theta) * std::sin(phi);
	const float z = std::cos(theta);

	return glm::vec3(x,y,z);
}

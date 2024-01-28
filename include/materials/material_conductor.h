#pragma once
#include <complex>

#include "material.h"

namespace rtt
{
	class Material_conductor : public Material {
		public: 
			int type;
			float alpha; //roughness - 0 for mirror
			glm::vec3 eta; // IOR (real)
			glm::vec3 kappa; // Absorption coeff
			glm::vec3 air_eta;
			glm::vec3 air_kappa;
			Material_conductor(float roughness, glm::vec3 eta, glm::vec3 kappa) : alpha(roughness), eta(eta), kappa(kappa), air_eta(glm::vec3(1.f)), air_kappa(glm::vec3(0.f)) {}

	float fresnel(std::complex<float> ior1, std::complex<float> ior2, float cosine) const {	
		const std::complex<float> cosi = cosine;

		const std::complex<float> cost = std::sqrt(1.f - (ior1 / ior2) * (ior1 / ior2) * (1.f - cosi * cosi)); // Snell's law		
		const std::complex<float> denominator = ior1 * cosi + ior2 * cost;
		const float rs_sqrt = std::abs( (ior1 * cosi - ior2 * cost) / denominator);
		const float rp_sqrt = std::abs( (ior1 * cost - ior2 * cosi) / denominator);

		return std::min(1.f, (rs_sqrt * rs_sqrt + rp_sqrt * rp_sqrt) / 2.f);
	}

	glm::vec3 fresnel(const float cosi) const {
		return glm::vec3(
			fresnel(std::complex<float>(air_eta.x, air_kappa.x), std::complex<float>(eta.x, kappa.x), cosi),
			fresnel(std::complex<float>(air_eta.y, air_kappa.y), std::complex<float>(eta.y, kappa.y), cosi),
			fresnel(std::complex<float>(air_eta.z, air_kappa.z), std::complex<float>(eta.z, kappa.z), cosi)
			);
	}
// This can be used for visible normal distribution and the pdf
	float normal_distribution(const glm::vec3 h) const { // from GGX
		//if (h.z <= 0) return 0.f; // xhi+(m,n) ---> <surface and microfacet normals> for debugging
		const float alpha2 = alpha * alpha;
		const float cosTheta2 = h.z * h.z;
		const float alpha_tan2 = alpha2 + (1.f - cosTheta2) / cosTheta2;
		
		return alpha2 / ((float)M_PI * cosTheta2 * cosTheta2 * alpha_tan2);
	}

	float G1(const glm::vec3 v) const { //, const glm::vec3 h) const {
		//if (glm::dot(v, h) / v.z <= 0.f) return 0.f; // xhi+(<v,m> / <v,n>) for debugging
		const float tanTheta2 = (1.f - v.z * v.z) / (v.z * v.z);
		
		return 2.f / (1.f + std::sqrt(1.f + alpha * alpha * tanTheta2));
	}

	glm::vec3 sample_PPG(BSDF &bsdf) const {
		if (bsdf.wo.z <= 0.f) { bsdf.inside = true; return glm::vec3(0.f); }
		return evaluate(bsdf) * bsdf.wo.z / (bsdf.wo_pdf * pdf(bsdf));
	}

	glm::vec3 sample(BSDF &bsdf, Sampler &sampler, bool ppg) const override {
		if (ppg) return sample_PPG(bsdf);		

		glm::vec3 wi = bsdf.wi;
		glm::vec3 h = sampleGGXnormalD(std::ref(sampler), alpha);
		bsdf.wo = glm::normalize(2.f * glm::dot(h, wi) * h - wi);
		
		float cosine = glm::dot(h, wi);
		if (cosine <= 0.f) {
			//std::cout << cosine << std::endl;
			return glm::vec3(0.f);
		}

		// evaluate
		const glm::vec3 F = fresnel(glm::dot(bsdf.wo, h));
		//const float D = normal_distribution(h);
		const float G = G1(wi) * G1(bsdf.wo);

		return glm::vec3((F * G * cosine) / (wi.z * h.z)); // cos * bsdf / pdf
	}

	glm::vec3 evaluate(BSDF &bsdf) const override {
		//if (bsdf.wo.z <= 0.f) return glm::vec3(0.f);
		glm::vec3 h = glm::normalize(bsdf.wi + bsdf.wo);
		if (h.z * bsdf.wo.z <= 0.f) return glm::vec3(0.f);
		float cosine = glm::dot(h, bsdf.wi);

		// evaluate
		const glm::vec3 F = fresnel(glm::dot(bsdf.wo, h));
		const float D = normal_distribution(h);
		const float G = G1(bsdf.wi) * G1(bsdf.wo);

		return glm::vec3(F * D * G / (4.f * bsdf.wi.z * bsdf.wo.z)); // bsdf
	}

	float pdf(BSDF &bsdf) const  override{
		//if (bsdf.wo.z <= 0.f) return 0.f;
		glm::vec3 h = glm::normalize(bsdf.wi + bsdf.wo);
		if (h.z * bsdf.wo.z <= 0.f) return 0.f;
		const float D = normal_distribution(h); 

		return D * h.z / (4.f * bsdf.wo.z);
	}

	};
}

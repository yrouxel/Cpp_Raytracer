#pragma once

#include "material.h"

namespace rtt
{
	class Material_dielectric : public Material {
		public: 
			int type;
			float alpha; //roughness - 0 for mirror
			float eta1; // IOR (real)
			float eta2;
			Material_dielectric(float roughness, float eta) : alpha(roughness), eta1(1.f), eta2(eta) {}

	float fresnel(float cosine, bool &tir, float tmp_eta1, float tmp_eta2) const {
		if (tmp_eta1 > tmp_eta2) {
			const float sinTIR = tmp_eta2 / tmp_eta1;
			const float sin_i = std::sqrt(1.f - cosine * cosine);
			if (sin_i > sinTIR) {
				tir = true;
				return 1.f;
			}
		}

		const float cost = std::sqrt(1.f - (tmp_eta1 / tmp_eta2) * (tmp_eta1 / tmp_eta2) * (1.f - cosine * cosine)); // Snell's law		
		const float denominator = tmp_eta1 * cosine + tmp_eta2 * cost;
		const float rs_sqrt = (tmp_eta1 * cosine - tmp_eta2 * cost) / denominator;
		const float rp_sqrt = (tmp_eta1 * cost - tmp_eta2 * cosine) / denominator;

		return std::min(1.f, (rs_sqrt * rs_sqrt + rp_sqrt * rp_sqrt) / 2.f);
	}
//This can be used for visible normal distribution and to calculate the pdf
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
		bool refraction = false;
		glm::vec3 h = glm::normalize(sampleGGXnormalD(std::ref(sampler), alpha));

		float tmp_eta1 = eta1;
		float tmp_eta2 = eta2;
		if (wi.z < 0.f) {
			h = -h; // reflect on the right side 
			std::swap(tmp_eta1, tmp_eta2);		
		}
		
		const float cosine = glm::dot(h, wi);
		if (cosine < 0.f) h = glm::vec3(-h.x, -h.y, h.z);

		bool tir = false;
		const float fres = fresnel(abs(cosine), std::ref(tir), tmp_eta1, tmp_eta2);

		if (fres >= sampler.next1D()) {
			// reflection
			//if (wi.z * glm::normalize(2.f * glm::dot(h, wi) * h - wi).z < 0.f) return glm::vec3(0.f);
			bsdf.wo = glm::normalize(2.f * glm::dot(h, wi) * h - wi);

			refraction = false;
		} else {
			// refraction
			if (cosine > 0.f) h = glm::vec3(-h.x, -h.y, h.z);

			const float ior = tmp_eta1 / tmp_eta2;
			float sign = 1.f;
			if (wi.z < 0) sign = -1.f;
			if (tir) { 
				//if (wi.z * glm::normalize(2.f * glm::dot(h, wi) * h - wi).z < 0.f) return glm::vec3(0.f);
				bsdf.wo = glm::normalize(2.f * glm::dot(h, wi) * h - wi); // TIR
				refraction = true;

			} else {
				//if (wi.z * glm::normalize((ior * cosine - sign * std::sqrt(1.f + ior * (cosine * cosine - 1.f))) * h - ior * wi).z > 0.f) return glm::vec3(0.f);
				bsdf.wo = glm::normalize((ior * cosine - sign * std::sqrt(1.f + ior * (cosine * cosine - 1.f))) * h - ior * wi);
				refraction = true;
			}
		}
		
		// evaluate 
		float F = fresnel(abs(glm::dot(bsdf.wo, h)), tir, tmp_eta1, tmp_eta2);
		if (refraction && !tir) F = 1.f - F; //std::cout << F << std::endl;}

		//const float D = normal_distribution(h); 
		const float G = G1(wi) * G1(bsdf.wo);

		bsdf.inside = ((tmp_eta1 > tmp_eta2 && (tmp_eta1 * tmp_eta2 > 1.f)) || (tmp_eta2 > tmp_eta1 && (tmp_eta1 * tmp_eta2 < 1.f)));
		return glm::vec3((F * G * cosine) / (bsdf.wi.z * h.z)); // cos * bsdf / pdf
	}

	glm::vec3 evaluate(BSDF &bsdf) const override {
		glm::vec3 h = glm::normalize(bsdf.wi + bsdf.wo);
		if (bsdf.wo.z * h.z < 0.f) return glm::vec3(0.f); // the outgoing ray is inside the object

		float tmp_eta1 = eta1;
		float tmp_eta2 = eta2;

		const float cosine = glm::dot(h, bsdf.wi); 
		bool refraction = bsdf.wi.z * bsdf.wo.z < 0.f; // reflection if on the same side of the surface
		bool tir = false;

		// evaluate 
		float F = fresnel(abs(glm::dot(bsdf.wo, h)), tir, tmp_eta1, tmp_eta2);
		if (refraction && !tir) F = 1.f - F;

		const float D = normal_distribution(h); 
		const float G = G1(bsdf.wi) * G1(bsdf.wo);

		bsdf.inside = ((tmp_eta1 > tmp_eta2 && (tmp_eta1 * tmp_eta2 > 1.f)) || (tmp_eta2 > tmp_eta1 && (tmp_eta1 * tmp_eta2 < 1.f)));
		return glm::vec3(F * D * G / (4.f * bsdf.wi.z * bsdf.wo.z)); // bsdf
	}

	float pdf(BSDF &bsdf) const override {
		glm::vec3 h = glm::normalize(bsdf.wi + bsdf.wo);
		if (bsdf.wo.z * h.z <= 0.f) return 0.f;
		const float D = normal_distribution(h); 

		return D * h.z / (4.f * bsdf.wo.z);
	}
/*
	glm::vec3 sample(BSDF &bsdf, Sampler &sampler) override {
		float xhi = sampler.next1D();
		glm::vec3 ks = bsdf.ks;
		glm::vec3 kd = bsdf.kd;
		if (ks + kd > 1.f) std::cout << "ks + kd > 1.f" << std::endl;
		
		float spec = ks.x + ks.y + ks.z;
		spec = spec / (spec + kd.x + kd.y + kd.z);

		if (xhi < spec) {
			//specular
		} else {
			//diffuse
		}

	}
*/

	};
}

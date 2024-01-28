#pragma once

#include "material.h"

namespace rtt
{
	class Material_normal : public Material {
		public: 
			int type;
			Material_normal() {}

		glm::vec3 sample(BSDF &bsdf, Sampler &sampler, bool ppg) const override {
			(void) bsdf; (void) sampler; (void) ppg;
			return glm::vec3(1.f);
		}

		glm::vec3 evaluate(BSDF &bsdf) const override {
			(void) bsdf;
			std::cout << "NOT IMPLEMENTED YET!" << std::endl;
			return glm::vec3(0.f);
		}

		float pdf(BSDF &bsdf) const  override{
			(void) bsdf;
			return 0.f;
		}
	};

	class Material_mirror : public Material {
		public: 
			int type;
			Material_mirror() {}
		
		glm::vec3 getReflection(glm::vec3 d) const {
			return glm::vec3(-d.x, -d.y, d.z);
		}

		glm::vec3 sample(BSDF &bsdf, Sampler &sampler, bool ppg) const override {
			(void) sampler; (void) ppg;
			bsdf.wo = getReflection(bsdf.wi);
			return glm::vec3(1.f);
		}

		glm::vec3 evaluate(BSDF &bsdf) const override {
			(void) bsdf;
			std::cout << "NOT IMPLEMENTED YET!" << std::endl;
			return glm::vec3(0.f);
		}
		float pdf(BSDF &bsdf) const  override{
			(void) bsdf;
			return 0.f;
		}
	};

	class Material_diffuse : public Material {
		public:
			int type;
			glm::vec3 color;
			Material_diffuse(glm::vec3 color) : color(color) {}

		glm::vec3 sample_PPG(BSDF &bsdf) const {
			if (bsdf.wo.z <= 0.f) { bsdf.inside = true; return glm::vec3(0.f); }
			//std::cout << bsdf.wo_pdf << " " << bsdf.wo.z << std::endl;
			return evaluate(bsdf) * bsdf.wo.z / (bsdf.wo_pdf * pdf(bsdf)); //return color * bsdf.wo.z / ((float) M_PI * bsdf.wo_pdf);
		}

		glm::vec3 sample(BSDF &bsdf, Sampler &sampler, bool ppg)const  override {
			if (ppg) return sample_PPG(bsdf);
			bsdf.wo = sampleHemisphere(std::ref(sampler));
			return color * bsdf.wo.z * 2.f; // bsdf = albedo \ pi ;;;; bsdf * cos / pdf
		}

		glm::vec3 evaluate(BSDF &bsdf) const override {
			(void) bsdf;
			return color / (float) M_PI ; // bsdf = albedo \ pi
		}

		float pdf(BSDF &bsdf) const  override {
			(void) bsdf;
			return 1.f / (2.f * M_PI);
		}
	};
}

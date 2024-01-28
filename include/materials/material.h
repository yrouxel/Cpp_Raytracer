#pragma once

namespace rtt
{
	class BSDF {
		public: 
			glm::vec3 wi;
			glm::vec3 wo;
			bool inside;
			float wo_pdf;
		BSDF() = default;
		BSDF(glm::vec3& wi, glm::vec3& wo, bool& inside) : wi(wi), wo(wo), inside(inside) {}
		BSDF(glm::vec3& wi, glm::vec3& wo, bool& inside, float &wo_pdf) : wi(wi), wo(wo), inside(inside), wo_pdf(wo_pdf) {}
	};

	class Material {
		public:
			int type;
			float roughness;

		Material() = default;
		Material(int type, float roughness) : type(type), roughness(roughness) {}
		virtual ~Material() = default;

		virtual glm::vec3 sample(BSDF &bsdf, Sampler &sampler, bool ppg) const = 0;
		virtual glm::vec3 evaluate(BSDF &bsdf) const = 0;
		virtual float pdf(BSDF &bsdf) const = 0;
	};
}

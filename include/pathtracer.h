#pragma once

#include "sampler.h"
#include "materials/material.h"
#include "materials/material_dielectric.h"
#include "materials/material_conductor.h"
#include "materials/perfect_material.h"
#include "plane.h"
#include "envmap.h"
#include "bounding_volume.h"

#include "binarytree.h"
#include <memory>

namespace rtt {

	const float roughness_threshold = 0.019f;

	bool isMirrorSurface(const std::unique_ptr<Material> &material) {
		return (material->type == 1) || 
			(((material->type == 3) || (material->type == 4) ) && (material->roughness < roughness_threshold));
	}

	class Vertex {
		public: 
			glm::vec3 wo;
			glm::vec3 position;
			glm::vec2 p;
			glm::vec3 throughput;
			Vertex(glm::vec3 wo, glm::vec3 position, glm::vec2 p, glm::vec3 throughput) : wo(wo), position(position), p(p), throughput(throughput) {}
	};

	void splatPPGSample (const int maxdepth, glm::vec3 intersectionNormal, std::vector<Vertex> vertices, std::vector<std::vector<glm::vec3>> &img, std::unique_ptr<BinaryTree> &binaryTree, std::mutex &) {
		for (int i = 0; i < maxdepth - 1; i++) {
			if (vertices[i].wo != glm::vec3(0.f) && vertices[i+1].wo != glm::vec3(0.f) && vertices[i].throughput != glm::vec3(0.f) && intersectionNormal != glm::vec3(0.f)) {
				glm::vec3 radiance = intersectionNormal * vertices[i].throughput;
				//mtx.lock();
				binaryTree->splatPosition(vertices[i].position, vertices[i].wo, img, 640, vertices[i].p, radiance);
				//mtx.unlock();
			}
		}
	}

	float mis_balance(float pdf1, float pdf2) {
		return pdf1 / (pdf1 + pdf2);
	}

	template<bool bsdf, bool nee>
	glm::vec3 Li(const std::unique_ptr<BVH>&, const Ray &, const EnvMap &, Sampler &, const int, const std::unique_ptr<Material> &, std::vector<std::vector<glm::vec3>> &, std::unique_ptr<BinaryTree> &, bool, std::mutex &){}


	template<>
	glm::vec3 Li<true, false>(const std::unique_ptr<BVH>& bvh, const Ray &ray, const EnvMap &envmap, Sampler &sampler, const int maxdepth, const std::unique_ptr<Material> &material, std::vector<std::vector<glm::vec3>> &img, std::unique_ptr<BinaryTree> &binaryTree, bool ppg, std::mutex &mtx)
	{
		glm::vec3 throughput(1.f);
		glm::vec3 color(0.f); // needed for mis later
		int depth = 0;

		Ray r = ray;
		bool inside = false;

		// Required for PPG -- otherwise not used
		std::vector<Vertex> vertices(maxdepth, Vertex(glm::vec3(0.f), glm::vec3(0.f), glm::vec2(0.f), glm::vec3(0.f)));	
		float wo_pdf = 1.f;
		glm::vec2 p(0.f);	

		while (depth < maxdepth){
			depth ++;

			Intersection its;
			its.normal = envmap.direction_to_texture_coords(r.direction);

			bvh->intersect(its, r);
			if (its.intersection) {
				if (material->type == 0) return abs(its.normal); // material_type = 0 --> normals

				Plane plane(its.normal);
				glm::vec3 wi = plane.toLocal(-r.direction);
				glm::vec3 wo = wi;

				if (ppg) {
					binaryTree->getQuadTreeDirection(its.position, sampler, envmap, img, 640, std::ref(wo), std::ref(wo_pdf), std::ref(p));
				}
				glm::vec3 local_wo = plane.toLocal(wo);
				BSDF b(wi, std::ref(local_wo), std::ref(inside), wo_pdf);
				const glm::vec3 bsdf = material->sample(std::ref(b), std::ref(sampler), ppg); // bsdf * cos / pdf

				r = Ray(its.position, plane.toGlobal(b.wo));
				throughput *= bsdf;
				inside = b.inside; // needed for conductors

				if (ppg) vertices[depth - 1] = Vertex(wo, its.position, p, throughput);

			} else {
				if (ppg) splatPPGSample(maxdepth, its.normal, vertices, img, binaryTree, mtx);

				// no intersection ----> get the value of the environment map
				if (inside) return color; // if the last bounce is refraction do not evaluate the envmap

				return color + its.normal * throughput;
			}
		}

		return color;
	}

	template<>
	glm::vec3 Li<false, true>(const std::unique_ptr<BVH>& bvh, const Ray &ray, const EnvMap &envmap, Sampler &sampler, const int maxdepth, const std::unique_ptr<Material> &material, std::vector<std::vector<glm::vec3>> &img, std::unique_ptr<BinaryTree> &binaryTree, bool ppg, std::mutex &mtx)
	{
		glm::vec3 throughput(1.f);
		glm::vec3 color(0.f); // needed for mis later
		int depth = 0;

		Ray r = ray;
		bool inside = false;

		// Required for PPG -- otherwise not used
		std::vector<Vertex> vertices(maxdepth, Vertex(glm::vec3(0.f), glm::vec3(0.f), glm::vec2(0.f), glm::vec3(0.f)));	
		float wo_pdf = 1.f;
		glm::vec2 p(0.f);

		while (depth < maxdepth){
			depth ++;

			Intersection its;
			its.normal = envmap.direction_to_texture_coords(r.direction);
			bool isMirror = isMirrorSurface(material); // perfect mirror

			bvh->intersect(its, r);
			if (its.intersection) {
				if (material->type == 0) return abs(its.normal); // material_type = 0 --> normals

				Plane plane(its.normal);
				glm::vec3 wi = plane.toLocal(-r.direction);
				glm::vec3 wo = wi;

				glm::vec3 d_world(0.f);
				float nee_pdf = 0.f;
				const glm::vec3 Li_nee = envmap.sampleEnvMap(std::ref(sampler), std::ref(d_world), std::ref(nee_pdf));
				glm::vec3 d = plane.toLocal(d_world);
				Intersection its_envmap;
				its_envmap.normal = envmap.direction_to_texture_coords(r.direction);
				Ray r_nee(its.position, plane.toGlobal(d));

				bvh->intersect(its_envmap, r_nee);
				BSDF b_nee(wi, std::ref(d), std::ref(inside), wo_pdf);
				if (!its_envmap.intersection && !b_nee.inside && !isMirror) { // check if envmap is occluded
					const glm::vec3 bsdf_nee = material->sample(std::ref(b_nee), std::ref(sampler), false);
					color += bsdf_nee * Li_nee; // without throughput as it is always 1 and is therefore used for bsdf ppg
				}

				if (ppg) {
					binaryTree->getQuadTreeDirection(its.position, sampler, envmap, img, 640, std::ref(wo), std::ref(wo_pdf), std::ref(p));
				}

				glm::vec3 local_wo = plane.toLocal(wo);
				BSDF b(wi, std::ref(local_wo), std::ref(inside), wo_pdf);

				const glm::vec3 bsdf = material->sample(std::ref(b), std::ref(sampler), ppg); // bsdf * cos / pdf
				throughput *= bsdf;

				r = Ray(its.position, plane.toGlobal(b.wo));
				inside = b.inside; // needed for conductors

				if (ppg) vertices[depth - 1] = Vertex(wo, its.position, p, throughput);

			} else if (depth == 1) {
				return its.normal; // * throughput = 1.f;
			} else {
				if (ppg) splatPPGSample(maxdepth, its.normal, vertices, img, binaryTree, mtx);

				if (inside || isMirror) return color;

				return color;
			}
		}

		return color;
	}

	template<>
	glm::vec3 Li<true, true>(const std::unique_ptr<BVH>& bvh, const Ray &ray, const EnvMap &envmap, Sampler &sampler, const int maxdepth, const std::unique_ptr<Material> &material, std::vector<std::vector<glm::vec3>> &img, std::unique_ptr<BinaryTree> &binaryTree, bool ppg, std::mutex &mtx)
	{
		glm::vec3 throughput(1.f);
		glm::vec3 color(0.f); // needed for mis later
		int depth = 0;

		Ray r = ray;
		bool inside = false;

		// MIS variables
		float bsdf_pdf = 1.f;

		// Required for PPG -- otherwise not used
		std::vector<Vertex> vertices(maxdepth, Vertex(glm::vec3(0.f), glm::vec3(0.f), glm::vec2(0.f), glm::vec3(0.f)));	
		float wo_pdf = 1.f;
		glm::vec2 p(0.f);

		while (depth < maxdepth){
			depth ++;

			Intersection its;
			its.normal = envmap.direction_to_texture_coords(r.direction);
			bool isMirror = isMirrorSurface(material); // perfect mirror

			bvh->intersect(its, r);
			Plane plane(its.normal);
			if (its.intersection) {
				if (material->type == 0) return abs(its.normal); // material_type = 0 --> normals

				glm::vec3 wi = plane.toLocal(-r.direction);
				glm::vec3 wo = wi;

				glm::vec3 d_world(0.f);
				float envmapPDF = 0.f;
				glm::vec3 Li_nee = envmap.sampleEnvMap(std::ref(sampler), std::ref(d_world), std::ref(envmapPDF));
				Li_nee /= envmapPDF;
				glm::vec3 d = plane.toLocal(d_world);
				Intersection its_envmap;
				Ray r_nee(its.position, plane.toGlobal(d));

				bvh->intersect(its_envmap, r_nee);
				BSDF b_nee(wi, std::ref(d), std::ref(inside), wo_pdf);
				if (!its_envmap.intersection && d.z >= 0.f && !b_nee.inside && !isMirror) { // check if envmap is occluded
					const glm::vec3 bsdf_nee = material->evaluate(std::ref(b_nee));

					float bsdf_pdf = material->pdf(b_nee);
					color += throughput * bsdf_nee * Li_nee * d.z * mis_balance(envmapPDF, bsdf_pdf);
				}

				if (ppg) {
					binaryTree->getQuadTreeDirection(its.position, sampler, envmap, img, 640, std::ref(wo), std::ref(wo_pdf), std::ref(p));
				}

				glm::vec3 local_wo = plane.toLocal(wo);
				BSDF b(wi, std::ref(local_wo), std::ref(inside), wo_pdf);
				const glm::vec3 bsdf = material->sample(std::ref(b), std::ref(sampler), ppg); // bsdf * cos / pdf

				r = Ray(its.position, plane.toGlobal(b.wo));

				bsdf_pdf = material->pdf(b);
				throughput *= bsdf;
				inside = b.inside; // needed for conductors

				if (ppg) vertices[depth - 1] = Vertex(wo, its.position, p, throughput);

			} else {
				if (ppg) splatPPGSample(maxdepth, its.normal, vertices, img, binaryTree, mtx);

				if (inside || isMirror) return color;

				float nee_pdf = envmap.getPDFfromDirection(r.direction);
				color += its.normal * throughput * mis_balance(bsdf_pdf, nee_pdf);
				return color; 
			}
		}

		return color;
	}
}

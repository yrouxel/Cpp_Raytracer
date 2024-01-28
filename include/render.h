#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <mutex>

#include <glm/glm.hpp>

#include "ray.h"
#include "camera.h"
#include "triangle.h"
#include "mesh.h"
#include "bvh.h"
#include "envmap.h"
#include "pathtracer.h"
#include "materials/material.h"

#include "omp.h"


namespace rtt //Realistic Ray Tracer
{
	std::mutex mtx;

	void saveimg(std::vector<std::vector<glm::vec3>>& img, int side, int p) {
		std::string imgName = "images/quadEnvmap" + to_string(p) + ".ppm";
		std::ofstream outfile (imgName, std::ios::out | std::ios::binary);  
		outfile << "P6\n" << side << " " << side << "\n255\n"; 
		
		for (int i = 0; i < side; ++i) { 
			for (int j = 0; j < side; ++j) { 
				outfile << (unsigned char)(std::min(1.f, img[i][j][0]) * 255)
					<< (unsigned char)(std::min(1.f, img[i][j][1]) * 255)
					<< (unsigned char)(std::min(1.f, img[i][j][2]) * 255);
			} 
		}
	 
		outfile.close();
	}

	void getMaterial(GUI& gui, std::unique_ptr<Material> &material) {
		switch (gui.curr_material) {
			case 0:
				material = std::make_unique<Material_normal>();
				material->type = gui.curr_material;
				material->roughness = gui.roughness;
				break;
			case 1: 
			{
				material = std::make_unique<Material_mirror>();
				material->type = gui.curr_material;
				material->roughness = gui.roughness;
				break;
			}
			case 2: 
			{
				material = std::make_unique<Material_diffuse>(glm::vec3(gui.diffColor));
				material->type = gui.curr_material;
				material->roughness = gui.roughness;
				break;
			}
			case 3: 
			{
				material = std::make_unique<Material_dielectric>(gui.roughness, gui.refIndex);
				material->type = gui.curr_material;
				material->roughness = gui.roughness;
				break;
			}
			case 4: 
			{
				material = std::make_unique<Material_conductor>(gui.roughness, gui.getMetalEta(gui.curr_metal), gui.getMetalKappa(gui.curr_metal));
				material->type = gui.curr_material;
				material->roughness = gui.roughness;
				break;
			}
			default:
				std::cout << "Something is wrong with materials" << std::endl;
				material = std::make_unique<Material_mirror>(); // return sth for default
				material->type = gui.curr_material;
			}
	}

	template<bool bsdf, bool nee, bool ppg>
	void renderNextFrame(const std::unique_ptr<BVH>& bvh, std::vector<glm::vec3> &buffer, unsigned int width, unsigned int height, rtt::Camera &camera, EnvMap& envmap, const int spp, Sampler &sampler, const int depth, const std::unique_ptr<Material> &material, std::unique_ptr<BinaryTree> &binaryTree, int iterationNumber) 
	{
		float screenWidthDiv  = camera.screenWidthDiv;
		float screenHeightDiv = camera.screenHeightDiv;
		float pixelX		  = camera.pixelX;
		float pixelY	      = camera.pixelY;
		std::vector<std::vector<glm::vec3>> img(width, std::vector<glm::vec3>(width, glm::vec3(0.f)));

		#pragma omp parallel
		{
			glm::vec3 color(0.f);
			#pragma omp for schedule(dynamic)
			for (unsigned int y = 0; y < height; y++) 
			{ 
				for (unsigned int x = 0; x < width; x++) 
				{
					for (int i = 0; i < spp; i++) { // for each pixel shoot many rays (spp) TODO: use parallel for loarge numbers
						float u = screenHeightDiv - pixelY * (sampler.next1D() + (float)y);
						float v = screenWidthDiv  + pixelX * (sampler.next1D() + (float)x);
						const Ray ray(camera.origin, camera.computeDirection(u, v));
						if constexpr(ppg) mtx.lock();
						color += Li<bsdf, nee>(bvh, ray, envmap, sampler, depth, material, img, binaryTree, ppg, std::ref(mtx));
						if constexpr(ppg) mtx.unlock();
					}
					buffer[width * y + x] = color / (float) spp;
					color = glm::vec3(0.f);
				}
			}
		}
		if constexpr(ppg)
		{
			mtx.lock();
			saveimg(img, width, iterationNumber);
			img = std::vector<std::vector<glm::vec3>>(width, std::vector<glm::vec3>(width, glm::vec3(0.f)));
			mtx.unlock();
		}
	}

	template<bool bsdf, bool nee>
	void renderNextFrame_PPG(const std::unique_ptr<BVH>& bvh, std::vector<glm::vec3> &buffer, std::vector<glm::vec3> &image, unsigned int width, unsigned int height, rtt::Camera &camera, EnvMap& envmap, const int spp, Sampler &sampler, const int depth, const std::unique_ptr<Material> &material, std::unique_ptr<BinaryTree> &binaryTree, int iterationNumber) 
	{
		for (int p = 0; p <= iterationNumber; p++) {
			cout << "PPG ..............................................." << p << endl;
			const int spps = pow(2, p); (void) spp;
			buffer = std::vector<glm::vec3>(width*height, glm::vec3(0.f)); // empty buffer in each iteration

			renderNextFrame<bsdf, nee, true>(bvh, buffer, width, height, camera, envmap, spps, sampler, depth, std::ref(material), binaryTree, p);

			std::swap(image, buffer);

			mtx.lock(); // any synchronization needed here if multiple threads
			binaryTree->resetQuadTree(width, p);
			binaryTree->refine(p);
			mtx.unlock();
		}
		mtx.lock();
		binaryTree->reset();
		mtx.unlock();
	}

	void render(const std::unique_ptr<BVH>& bvh, vector<vec3> &image, GUI& gui, EnvMap& envmap, Sampler& sampler) {
		std::vector<glm::vec3> buffer(gui.width*gui.height);

		while (!gui.quit) {
			if (gui.changed || gui.changedMap) {
				gui.changed = false;

				//TODO: use pipeline double/triple buffering
				Camera camera(gui.width, gui.height, gui.angleFOV, gui.cameraOrigin, gui.cameraAngle);
				if (gui.changedMap) {
					gui.changedMap = false;
					envmap.calculateEnvironmentMap(gui.envmapExposure);
				}
				
				std::unique_ptr<Material> material;
				getMaterial(gui, std::ref(material));
				std::unique_ptr<BinaryTree> binaryTree = std::make_unique<BinaryTree>(bvh->min_, bvh->max_, gui.c, gui.t);

				if (gui.ppg) { // practical path guiding
					switch (gui.mode) {
						case 0: // BSDF
							renderNextFrame_PPG<true, false>(bvh, buffer, image, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), gui.iterationNumber);
							std::cout << "BSDF ..." << std::endl;
							break;
						case 1: // NEE
							renderNextFrame_PPG<false,true>(bvh, buffer, image, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), gui.iterationNumber);
							std::cout << "NEE ..." << std::endl;
							break;
						case 2: // MIS
							renderNextFrame_PPG<true, true>(bvh, buffer, image, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), gui.iterationNumber);
							std::cout << "MIS ..." << std::endl;
							break;
						default:
							std::cout << "Something is wrong with the modes!" << std::endl;
					}
				} else { // path tracing
					switch (gui.mode) {
						case 0: // BSDF
							std::cout << "BSDF begins ..." << std::endl;
							renderNextFrame<true, false, false>(bvh, buffer, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), 0);
							std::cout << "BSDF completed ..." << std::endl;
							std::swap(image, buffer);
							break;
						case 1: // NEE
							std::cout << "NEE begins ..." << std::endl;
							renderNextFrame<false, true, false>(bvh, buffer, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), 0);
							std::cout << "NEE completed..." << std::endl;
							std::swap(image, buffer);
							break;
						case 2: // MIS
							std::cout << "MIS begins ..." << std::endl;
							renderNextFrame<true, true, false>(bvh, buffer, gui.width, gui.height, camera, envmap, std::pow(2.f, gui.spp), sampler, gui.depth, std::ref(material), std::ref(binaryTree), 0);
							std::cout << "MIS completed ..." << std::endl;
							std::swap(image, buffer);
							break;
						default:
							std::cout << "Something is wrong with the modes!" << std::endl;
					}
				}
			}	
		}
		return;
	}
}

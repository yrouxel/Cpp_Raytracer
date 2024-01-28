#include <iostream>
#include <vector>
#include <math.h> 
#include <string>
#include <memory>
#include <fstream> //save image
#include <string.h> 

#include "envmap.h"
#include "quadtree.h"
#include "binarytree.h"
#include "sampler.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt
#include <glm/gtx/transform.hpp>

#include "render.h"
#include "parser.h"

#include "mesh.h"
#include "envmap.h"
#include "bvh.h"

#include "materials/material.h"
#include "materials/perfect_material.h"

void saveImage(std::vector<std::vector<glm::vec3>>& image, int side) {
	std::string imageName = "images/quadEnvmap.ppm";
	std::ofstream outfile (imageName, std::ios::out | std::ios::binary);  
	outfile << "P6\n" << side << " " << side << "\n255\n"; 
	
	for (int i = 0; i < side; ++i) { 
		for (int j = 0; j < side; ++j) { 
			outfile << (unsigned char)(std::min(1.f, image[i][j][0]) * 255)
				<< (unsigned char)(std::min(1.f, image[i][j][1]) * 255)
				<< (unsigned char)(std::min(1.f, image[i][j][2]) * 255);
		} 
	}
 
	outfile.close();
}

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


int main() {
	string dir_path = "scenes";
	string scene_name = "teapot";	

	vector<vec3> image(640*480); 
	vector<vec3> buffer(640*480); 

	const auto bvh = rtt::createBVH(dir_path + "/" + scene_name + "/", scene_name);

	rtt::Camera camera(640, 480, 60.0f * M_PI /180.f, glm::vec3{0.f, 0.f, -2.f}, glm::vec3{0.f, 0.f, 0.f});

	std::unique_ptr<rtt::Material> material = std::make_unique<rtt::Material_diffuse>(glm::vec3(0.3f));
	material->type = 2;
	Sampler sampler;
////////////////////////////////////////////////////////////////

	EnvMap envmap("scenes/envmap3.exr", -1.f);
	//Sampler sampler;

	//BinaryTree binaryTree;
	std::unique_ptr<BinaryTree> binaryTree = std::make_unique<BinaryTree>(bvh->min_, bvh->max_);

	glm::vec3 intersectionPoint(0.8f);
	glm::vec3 wo(0.f);
	//float wo_pdf = 1.f;
	glm::vec2 p(0.f);

	//int side = 640;
	//std::vector<std::vector<glm::vec3>> img(side, std::vector<glm::vec3>(side, glm::vec3(0.f))); 

	rtt::renderNextFrame_PPG<true, false>(bvh, std::ref(buffer), std::ref(image) , 640, 480, std::ref(camera), std::ref(envmap), std::pow(2.f, 3), std::ref(sampler), 5, std::ref(material), std::ref(binaryTree), 5);

	/*// Sample only environment map
	for (int i = 0; i < 1; i ++) { // iteration cycle

		for (int s = 0; s < 10000000; s++) { // samples per iteration
			intersectionPoint = glm::vec3(sampler.next1D(), sampler.next1D(), sampler.next1D());
			binaryTree->getQuadTreeDirection(intersectionPoint, std::ref(sampler), std::ref(envmap), std::ref(img), side, std::ref(wo), std::ref(wo_pdf), std::ref(p));
			glm::vec3 radiance = envmap.direction_to_texture_coords(wo);
			binaryTree->splatPosition(intersectionPoint, wo, std::ref(img), side, p, radiance);
		}
		saveimg(std::ref(img), side, i);
		img = std::vector<std::vector<glm::vec3>>(side, std::vector<glm::vec3>(side, glm::vec3(0.f)));

		binaryTree->resetQuadTree(side, i);
		binaryTree->refine(i);
	}

	//saveImage(std::ref(img), side);
	//img = std::vector<std::vector<glm::vec3>>(side, std::vector<glm::vec3>(side, glm::vec3(0.f)));
*/
	return 0;
}

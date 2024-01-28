#include <iostream>
#include <vector>
#include <math.h> 
#include <string> 
#include <limits>
#include <thread>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt
#include <glm/gtx/transform.hpp>

#include "render.h"
#include "parser.h"

#include "mesh.h"
#include "envmap.h"
#include "bvh.h"

#include "materials/material.h"
#include "materials/material_dielectric.h"

int main(){
	string dir_path = "scenes";
	string scene_name = "teapot";	
	EnvMap envmap(dir_path + "/" + scene_name + "/" + scene_name + ".exr", 0.f);
	Sampler sampler;


	std::unique_ptr<rtt::Material> material = std::make_unique<rtt::Material_conductor>(0.f, glm::vec3{0.048778, 0.059582, 0.049317}, glm::vec3{4.5264, 3.5974, 2.8545});;

	// TODO: allocate memory when initializing image
	vector<vec3> image(640 * 480); 

	const auto bvh = rtt::createBVH(dir_path + "/" + scene_name + "/", scene_name);

	std::unique_ptr<BinaryTree> binaryTree = std::make_unique<BinaryTree>(bvh->min_, bvh->max_);

	const unsigned int frames = 16;
	const glm::vec3 origin(-6.f, 3.f, -5.f);
	const glm::vec3 angle(0.f, 45.f, 0.f);
	rtt::Camera camera(640, 480, 90.f, origin, angle);

	auto t1 = chrono::high_resolution_clock::now();
	for (unsigned int i = 0; i < frames; i++)
		rtt::renderNextFrame<true, false, false>(bvh, image, 640, 480, camera, envmap, 8, sampler, 5, material, std::ref(binaryTree), 0);	

	// for (unsigned int i = 0; i < 20; i++)
	// {
	// 	rtt::renderNextFrame(mesh_values, bvh, image, 640, 480, camera, envmap, 0, sampler, 5, material, false, std::ref(binaryTree), 0, false);	
	// 	camera.origin.x += 1.0;
	// }
	// for (unsigned int i = 0; i < 20; i++)
	// {
	// 	rtt::renderNextFrame(mesh_values, bvh, image, 640, 480, camera, envmap, 0, sampler, 5, material, false, std::ref(binaryTree), 0, false);	
	// 	camera.origin.y += 1.0;
	// }	
	// for (unsigned int i = 0; i < 20; i++)
	// {
	// 	rtt::renderNextFrame(mesh_values, bvh, image, 640, 480, camera, envmap, 0, sampler, 5, material, false, std::ref(binaryTree), 0, false);	
	// 	camera.origin.z += 1.0;
	// }

	auto t2 = chrono::high_resolution_clock::now();
	auto ms_count = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
	std::cout << "total time: " << ms_count << " ms\n";
	std::cout << "average time per frame: " << (ms_count/frames) << " ms\n";

	cout << "Done!" << endl;
	return 0;
}

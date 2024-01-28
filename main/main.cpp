#include <iostream>
#include <vector>
#include <math.h> 
#include <string> 
#include <limits>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp> // to_string mat4

#include "ray.h"
#include "camera.h"
#include "triangle.h"
#include "mesh.h"
#include "bvh.h"
#include "envmap.h"

#include "render.h"
#include "parser.h"

#include "materials/material_conductor.h"
#include "materials/material_dielectric.h"

int main(){
	GUI gui;
	string dir_path = "scenes/basic_scenes";
	string scene_name = "teapot";	
	EnvMap envmap(dir_path + "/" + scene_name + "/" + scene_name + ".exr", gui.envmapExposure);
	Sampler sampler;

	// TODO: allocate memory when initializing image
	vector<vec3> image(gui.width * gui.height); 

	const auto bvh = rtt::createBVH(dir_path + "/" + scene_name + "/", scene_name);

	//mat4 rot = rotate(mat4(1.0f), 45.f, vec3(1.0, 0.0, 0.0));
	//mat4 rot2 = rotate(mat4(1.0f), 30.f, vec3(0.0, 1.0, 0.0));
	//mat4 objT = translate(mat4(1.f), vec3(0.f, 0.f, 1.f));
	
	//for (unsigned int i =0; i < out_vertices.size(); i ++) {
	//	out_vertices[i] = vec3(objT  * rot * rot2 * vec4(out_vertices[i], 1.0));
	//}

	// Do ray tracing and display on GUI with threrender(ads
	vector<thread> threads;

	threads.push_back(thread(rtt::render, std::ref(bvh), std::ref(image), std::ref(gui), std::ref(envmap), std::ref(sampler)));
	view_gui(image, gui);

	for(auto& thread : threads){
		thread.join();
	}
	
	cout << "Done!" << endl;
	return 0;
}
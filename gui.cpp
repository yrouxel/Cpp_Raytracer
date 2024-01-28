#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <math.h> 
#include <fstream> //save image
#include <string.h> 
#include <glm/gtx/string_cast.hpp> // to_string mat4
//imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "gui.h"

using namespace glm;
using namespace std;

// Display image in the background
bool loadTextureFromImage(GLuint &out_texture, const int w, const int h, const vector<vec3> &image) {
	unsigned char image_data[w * h * 3];
	if (image_data == NULL) return false;
	
	//Create Texture
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// PARSE VECTOR TO UNSIGNED CHAR3
	for (int i = 0; i < w * h; i++) 
	{
		image_data[3 * i    ] = (unsigned char)(std::min(1.f, image[i][0]) * 255);
		image_data[3 * i + 1] = (unsigned char)(std::min(1.f, image[i][1]) * 255);
		image_data[3 * i + 2] = (unsigned char)(std::min(1.f, image[i][2]) * 255);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	out_texture = std::move(image_texture);

	return true;
}

void saveImage(const vector<vec3>& image, const GUI& gui, const string sceneName) {
	string imageName = "images/" + sceneName + "_" + to_string(gui.angleFOV) + "_" + to_string(gui.cameraOrigin) + "_image.ppm";
	ofstream outfile (imageName, ios::out | ios::binary);  
	outfile << "P6\n" << gui.width << " " << gui.height << "\n255\n"; 
	
	for (unsigned int i = 0; i < gui.width * gui.height; ++i) { 
		outfile << (unsigned char)(std::min(1.f, image[i][0]) * 255) << 
			(unsigned char)(std::min(1.f, image[i][1]) * 255) << 
			(unsigned char)(std::min(1.f, image[i][2]) * 255); 
	}
 
	outfile.close();
}

void view_gui(const vector<vec3> &image, GUI& gui) {
	int width = gui.width;
	int height = gui.height;

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Rendered Image", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return;
	}

	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");

	while (!glfwWindowShouldClose(window)) {
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// Tell OpenGL a new frame is about to begin
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		GLuint texture = 0;
		loadTextureFromImage(texture, width, height, image);

		ImGui::Begin("Properties");
		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)(intptr_t)texture, ImVec2(0,0), ImVec2(width,height), ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE);

		if (ImGui::TreeNode("Camera setup")) {
			gui.changed |= ImGui::SliderAngle("FoV", &gui.angleFOV, 1.f, 179.f); // constrains because of tan
			// the constrain values are set for meshes with vertices in range (0, 1)
			gui.changed |= ImGui::SliderFloat("Camera origin X", &gui.cameraOrigin.x, -15, 15);
			gui.changed |= ImGui::SliderFloat("Camera origin Y", &gui.cameraOrigin.y, -15, 15);
			gui.changed |= ImGui::SliderFloat("Camera origin Z", &gui.cameraOrigin.z, -15, 25);

			gui.changed |= ImGui::SliderAngle("Camera angle X", &gui.cameraAngle.x, -180.f, 180.f);
			gui.changed |= ImGui::SliderAngle("Camera angle Y", &gui.cameraAngle.y, -180.f, 180.f);
			gui.changed |= ImGui::SliderAngle("Camera angle Z", &gui.cameraAngle.z, -180.f, 180.f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Environment map setup")) {
			gui.changedMap |= ImGui::SliderFloat("Exposure", &gui.envmapExposure, -10.f, 10.f);
			gui.changed |= gui.changedMap; // TODO: change it. It is a quick fix for now. Current behaviour: when exposure is changed the image is not updated
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Scene setup")) {
			gui.changed |= ImGui::SliderInt("SPP: 2^s", &gui.spp, 0, 10);
			gui.changed |= ImGui::SliderInt("Max length of path", &gui.depth, 1, 15);
			ImGui::Dummy(ImVec2(15,15));

			gui.changed |= ImGui::Combo("Materials", &gui.curr_material, gui.materials, IM_ARRAYSIZE(gui.materials));
	
			if (gui.curr_material == 2) { // Diffuse
				gui.changed |= ImGui::ColorEdit3("Diffuse color", &gui.diffColor.x);
			}
			if (gui.curr_material == 3 ) { // Cook Torrance: Dielectric
				gui.changed |= ImGui::SliderFloat("Roughness", &gui.roughness, 0.f, 3.f);
				gui.changed |= ImGui::SliderFloat("Refractive Index", &gui.refIndex, 0.1f, 3.5f);
			}
			if (gui.curr_material == 4) { // Cook Torrance: Conductor
				gui.changed |= ImGui::SliderFloat("Roughness", &gui.roughness, 0.f, 3.f);
				gui.changed |= ImGui::Combo("Metals", &gui.curr_metal, gui.metals, IM_ARRAYSIZE(gui.metals));
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Path construction techniques")) {
			if (ImGui::RadioButton("BSDF", gui.mode == gui.Mode_BSDF)) { gui.mode = gui.Mode_BSDF; gui.changed = true; } ImGui::SameLine();
			if (ImGui::RadioButton("NEE", gui.mode == gui.Mode_NEE)) { gui.mode = gui.Mode_NEE; gui.changed = true; } ImGui::SameLine();
			if (ImGui::RadioButton("MIS (BSDF + NEE)", gui.mode == gui.Mode_MIS)) { gui.mode = gui.Mode_MIS; gui.changed = true; }
			ImGui::Dummy(ImVec2(15,15));

			gui.changed |= ImGui::Checkbox("PPG enabled ", &gui.ppg);
			if (ImGui::TreeNode("PPG properties")) {
				ImGui::SliderInt("Iteration Number", &gui.iterationNumber, 0, 10);
				ImGui::InputInt("Binary Tree split constatnt", &gui.c);
				ImGui::SliderFloat("Quad Tree flux threshold", &gui.t, 0.0000001f, 1.0f);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(15,15));
		if (ImGui::Button("Save Image"))
			saveImage(image, gui, "cube");
			
		ImGui::End();

		ImGui::Render();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
       		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events
		glfwSwapBuffers(window);
		glfwPollEvents();

		glDeleteTextures(1, &texture);
	}
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	gui.quit = true;
}

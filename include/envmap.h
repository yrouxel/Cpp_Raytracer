#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <fstream>  
#include <stdlib.h>     /* div, div_t */
#include "omp.h"
#include <glm/gtx/string_cast.hpp> // to_string mat4

#include "sampler.h"  

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

// Only for debugging
void saveImage(const std::vector<glm::vec3>& image, int width, int height) {
	std::ofstream outfile ("images/test.ppm", std::ios::out | std::ios::binary);  
	outfile << "P6\n" << width << " " << height << "\n255\n"; 
	
	for (auto i = 0; i < width * height; ++i) { 
		outfile << (unsigned char)(std::min(1.f, image[i][0]) * 255) << 
			(unsigned char)(std::min(1.f, image[i][1]) * 255) << 
			(unsigned char)(std::min(1.f, image[i][2]) * 255); 
	}
 
	outfile.close();
}

class EnvMap {
	public:
		int width;
		int height;
		float exposure;
		std::vector<glm::vec3> envmap;
		std::vector<glm::vec3> envmapOriginal;
		std::vector<float> envmapPDF;
		std::vector<float> envmapSolidAnglePDF;
	
		EnvMap(std::string name, float exposure) : exposure(exposure) {
			std::cout << "Extracting environment map ... " << std::endl;

			const char* input = name.c_str();
			float* out; // width * height * RGBA
			const char* err = NULL; // or nullptr in C++11

			int ret = LoadEXR(&out, &width, &height, input, &err);

			if (ret != TINYEXR_SUCCESS) {
				if (err) {
					fprintf(stderr, "ERR : %s\n", err);
					FreeEXRErrorMessage(err); // release memory of error message.
				}
			} else {
				int c = 0;
				glm::vec3 rgba (0.f);
				std::vector<glm::vec3> img(width * height, rgba);
				std::vector<float> pdfImg(width * height, 0.f);

				for (int i = 0; i < width * height; i++) {
					rgba[0] = out[4 * i + 0];
					rgba[1] = out[4 * i + 1];
					rgba[2] = out[4 * i + 2];
					img[c] = rgba;
					c++;
				}
				//saveImage(img, width, height);
				envmap = img;
				envmapOriginal = img;
				envmapPDF = pdfImg;
				envmapSolidAnglePDF = pdfImg;
				std::copy(img.begin(), img.end(), envmapOriginal.begin());
				free(out); // release memory of image data
			}
			calculateEnvironmentMap(exposure);

		}
		
		void calculateEnvironmentMap(float exp) 
		{
			exposure = exp;
			float luminance = 0.f;
			float solidAngleLuminance = 0.f;
			float k = std::pow(2.f, exposure + 2.47393f);
			// TODO: use parallel
			for (unsigned int i = 0; i < envmap.size(); i++) {
				envmap[i] = glm::min(envmapOriginal[i] * k / 3.f, glm::vec3(1.f));
				luminance += (envmap[i].x + envmap[i].y + envmap[i].z) / 3.f;
				envmapPDF[i] = luminance;
			}
			
			// normalize envmap PDF
			for (unsigned int i = 0; i < envmapPDF.size(); i++) {
				envmapPDF[i] /= luminance; // cmf --> in texture space
				envmapSolidAnglePDF[i] = getSolidAnglePDF(i);
				solidAngleLuminance += envmapSolidAnglePDF[i];
			}

			#pragma omp parallel for simd
			for (unsigned int i = 0; i < envmapSolidAnglePDF.size(); i++) {
				envmapSolidAnglePDF[i] /= solidAngleLuminance; // cmf --> in sphere space
			}
		}

		// evaluatuion
		glm::vec3 direction_to_texture_coords_GL(glm::vec3 d) const {
			const float inv_norm = 0.5f / std::sqrt(d[0] * d[0] + d[1] * d[1] + (d[2] + 1.f) * (d[2] + 1.f));
			float x = d[0] * inv_norm + 0.5f;
			float y = d[1] * inv_norm + 0.5f;
			x = std::clamp(int(width * x), 0, width - 1);
			y = std::clamp(int(height * y), 0, height - 1);
			return envmap[y * width + x];
		}

		int direction_to_texelID(glm::vec3 d) const {
			const float ty = std::acos(d.y) / M_PI; // theta = acos(d.y)
			float tx = std::atan2(d.z, d.x) / (2.f * M_PI); // phi = atan(d.z / d.x)
			tx += tx < 0.f; // if phi < 0 --> phi += 2 * phi

			const float x = std::clamp(int(width * tx), 0, width - 1);
			const float y = std::clamp(int(height * ty), 0, height - 1);

			return y * width + x;
		}

		// evaluation
		glm::vec3 direction_to_texture_coords(glm::vec3 d) const {
			int texelID = direction_to_texelID(d);

			return envmap[texelID];
		}

		glm::vec3 texelID_to_direction(int texelID) const {
			div_t res = div(texelID, width);
			float x = res.rem; // %
			float y = res.quot; // //

			float theta = M_PI * y / height;
			float phi = 2 * M_PI * x / width;

			return glm::normalize(
					glm::vec3(std::sin(theta) * std::cos(phi),
						 std::cos(theta),
						 std::sin(theta) * std::sin(phi))
				);
		}

		int getTexelID(Sampler &sampler) const {
			const float xhi = sampler.next1D();
			long unsigned int i = 0;
			while (i < envmapPDF.size()) { // can be optimized by median split
				if (envmapPDF[i] >= xhi) return i;
				i++;
			}
			std::cout << "[DEBUG] Sampled end of envmap, which should be impossible..." << std::endl;
			return envmapPDF.size() - 1;
		}

		float getSolidAnglePDF(int texelID) const {
			float pdfTexel = envmapPDF[texelID];
			if (texelID != 0) pdfTexel = envmapPDF[texelID] - envmapPDF[texelID - 1];

			float y = div(texelID, width).quot;
			float theta = M_PI * y / height; // theta in [0, pi]
			if (y > height) std::cout << "[DEBUG] Larger y than height" << std::endl;
			float sinTheta = std::sin(theta);
			if (sinTheta == 0.f) return 0.00001f;

			return (pdfTexel * width * height) / (2.f * M_PI * M_PI * sinTheta); // pdf
		}

		glm::vec3 sampleEnvMapUniformly(Sampler &sampler, glm::vec3 &d) const {
			d = sampleHemisphere(sampler);
			return direction_to_texture_coords(d) * (2.f * (float) M_PI); //eval / pdf
		}

		glm::vec3 sampleEnvMap(Sampler &sampler, glm::vec3 &d, float &pdf) const {
			int texelID = getTexelID(sampler);
			pdf = envmapSolidAnglePDF[texelID]; // pdf
			d = texelID_to_direction(texelID);

			return envmap[texelID]; //eval
		}

		float getPDFfromDirection(glm::vec3 dir) const {
			int texelID = direction_to_texelID(dir);
			return envmapSolidAnglePDF[texelID];
		}
};


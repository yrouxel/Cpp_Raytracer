#include <vector>
#include <glm/glm.hpp>

using namespace glm;
using namespace std;

class GUI {
	public:
		bool changed;
		bool changedMap;
		bool quit;

		unsigned int width;
		unsigned int height;

		float angleFOV;
		glm::vec3 cameraOrigin;
		glm::vec3 cameraAngle;

		float envmapExposure;

		int spp;
		int depth;

		const char* materials[5] = {"Normals", "Perfectly Flat Mirror", "Perfect Diffuse", "Cook-Torrance: Dielectric", "Cook-Torrance: Conductor"};
		int curr_material;

		float roughness;
		glm::vec3 diffColor;
		float refIndex;
		
		const char* metals[5] = {"Silver", "Gold", "Copper", "Zinc", "Cobalt"};
		int curr_metal;

		bool ppg;
		int iterationNumber;

		enum Mode { Mode_BSDF, Mode_NEE, Mode_MIS};
		int mode;

		// PPG variables
		int c;
		float t;

		GUI() : changed(true), quit(false), width(640), height(480), angleFOV(60.0f * M_PI /180.f), cameraOrigin(vec3{0.f, 0.f, -2.f}), cameraAngle(vec3{0.f, 0.f, 0.f}), envmapExposure(-1.f), spp(0), depth(5), curr_material(2), roughness(0.02f), diffColor(0.3f), refIndex(1.5f), curr_metal(0), ppg(false), iterationNumber(4), mode(0), c(12000), t(0.01f) {}

		// r = 665nm; g = 550nm; b = 470nm
		glm::vec3 getMetalEta(int metal) {
			switch (metal) {
				case 0: // Silver
					return glm::vec3{0.048778, 0.059582, 0.049317};
				case 1: // Gold
					return glm::vec3{0.13878f, 0.42415f, 1.3148f};
				case 2: // Copper
					return glm::vec3{0.21878, 1.0066, 1.2493};
				case 3: // Zinc
					return glm::vec3{1.2828, 0.92943, 0.72459};
				case 4: // Cobalt
					return glm::vec3{2.2580, 2.0524, 1.8065};
				default:
					return glm::vec3(0.f);
			}
				
		}

		glm::vec3 getMetalKappa(int metal) {
			switch (metal) {
				case 0: // Silver
					return glm::vec3{4.5264, 3.5974, 2.8545};
				case 1: // Gold
					return glm::vec3{3.7466f, 2.4721f, 1.8534f};
				case 2: // Copper
					return glm::vec3{3.8030, 2.5823, 2.4771};
				case 3: // Zinc
					return glm::vec3{6.0033, 4.9751, 4.2089};
				case 4: // Cobalt
					return glm::vec3{4.2940, 3.8242, 3.4035};
				default:
					return glm::vec3(0.f);
			}
				
		}
};

void view_gui(const vector<vec3> &image, GUI & gui);

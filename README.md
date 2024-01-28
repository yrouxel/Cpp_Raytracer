# Graphics_And_Game_Development

This is a simple plain cpp ray tracer developed for the practical course Graphics and Game Development. It has been tested and runs on Ubuntu and Mac OS. The ray tracing is entirely CPU-based.

**Compile and run**  
First you need to clone [glm](https://github.com/g-truc/glm) and [glfw](https://github.com/glfw/glfw) repositories and place them in the corresponding folders in the lib directory. The name of the scene can be changed from the file ```main.cpp```. Note that the environment maps for the scenes are not uploaded in the repository. An environment map can be downloaded from [PolyHaven](https://polyhaven.com/hdris) and placed in the correponding scene folder and renamed exactly as the scene name.

```
$ mkdir build
$ cd build
$ cmake ..
$ make -j5
$ cd ..
$ ./render
```
The application features a GUI which could easily change the scene setup. The GUI can adjust the following properties:
 - Camera setup:
	- Field of View
	- Camera origin (3D)
	- Camera rotation (3D)
 - Environment map setup:
	- Exposure
 - Scene setup:
	- Samples per pixel (spp)
	- Length of path / Maximum depth
	- Materials
		- Color
		- Index of refraction
		- Roughness
		- Type of metal
	- BSDF
	- NEE 
	- MIS
	- [Practical Path Guiding](https://studios.disneyresearch.com/wp-content/uploads/2019/03/Practical-Path-Guiding-for-Efficient-Light-Transport-Simulation.pdf)
		- Iteration number
		- Binary Tree split constant
		- Quad Tree flux threshold
 - Save image

**Features** 
- Simple obj loading and xml support
- Light source: Environment Map
- BVH Accelaration structure
- Multi-processing
- Materials support
- Path Tracing
	- BSDF Sampling
	- Next Event Estimation (NEE) Sampling 
	- Multiple Importance Sampling (MIS) with Balanced Heuristic
	- Practical Path Guiding (PPG)

**Supported Materials**
- Normals
- Perfect diffuse
- Perfect flat mirrors
- Rough conductors*
- Rough dielectrics*

*Rough conductors and dielectrics are implemented according to the paper [Microfacet Models for Refraction through Rough Surfaces](https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf)


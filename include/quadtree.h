#pragma once
#include <fstream> //save image
#include <string.h> 
#include <memory>
#include <atomic>         // std::atomic, std::atomic_flag, ATOMIC_FLAG_INIT

#include "sampler.h"

float flux_threshold = 0.01f;

class Point {
	public:
		float x;
		float y;
		Point(float x, float y) : x(x), y(y) {}

		bool operator<= (Point p) {
			return x <= p.x && y <= p.y;
		}

		bool operator> (Point p) {
			return x > p.x && y > p.y;
		}
};

class BBox {
	public:
		Point a; // upper left point
		Point b; // lower right point
		float length; // side of the square
		BBox(Point a, Point b) : a(a), b(b), length(b.x - a.x) {}

		std::vector<BBox> splitBBox() { // i: 0 - upper left, 1 - upper right, 2 - lower left, 3 - lower right
			float halfLength = length / 2.f;
			std::vector<BBox> bboxes;
			bboxes.push_back(BBox(a, Point(a.x + halfLength, a.y + halfLength)));
			bboxes.push_back(BBox(Point(a.x + halfLength, a.y), Point(b.x, a.y + halfLength)));
			bboxes.push_back(BBox(Point(a.x, a.y + halfLength), Point(a.x + halfLength, b.y)));
			bboxes.push_back(BBox(Point(a.x + halfLength, a.y + halfLength), b));
			return bboxes;
		}

		bool containsPoint(Point p) {
			return a <= p && b > p;
		}	
};

class DNode {
	public:
		std::vector<DNode> children; // 4 or 0 children inside 1 Node
		std::atomic<float> flux; // sum of childrens' fluxes
		int level;
		BBox b;
		DNode(std::atomic<float> flux, int level, BBox b) : flux(flux.load()), level(level), b(b) {}
		DNode(const DNode &dn) : children(dn.children), flux(std::atomic<float>(dn.flux.load())), level(dn.level), b(dn.b) {} // copy constructor

		void mergeDNode() {
			if (!isLeaf()) {
				for (int i = 0; i < 4; i++) {
					children[i].mergeDNode();
				}
				children.clear();
			}
		}

		DNode& operator=(const DNode &dn) {
			if (this == &dn) return *this;
			DNode n = dn; 
			if (!n.isLeaf()) {
				if (isLeaf()) {
					for (int i = 0; i < 4; i ++) {
						DNode c = n.children[i];
						children.push_back(c);
					}
				}
				for (int i = 0; i < 4; i ++) {
					DNode c = n.children[i];
					children[i]=(c);
				}
			} else {
				mergeDNode();
			}
			flux.exchange(n.flux.load());
			level = n.level;
			b = n.b;
			return *this;
		}

		bool isLeaf() {
			return children.size() == 0;
		}

		void splitLeafNode() {
			std::vector<BBox> bboxes = b.splitBBox();
			if (children.size() != 0) std::cout << "[DEBUG] Tried to split node with " << children.size() << " children" << std::endl;
			for (int i = 0; i < 4; i++) {
				DNode node(std::atomic<float>(flux.load() / 4.f), level+1, bboxes[i]);
				children.push_back(node);
			}
		}

		void recordDirection(Point p, float f) {
			if (b.containsPoint(p)) {
				flux.store(flux.load() + f);

				if (!isLeaf()) {
					if (children.size() != 4) std::cout << "[DEBUG] Tried to record direction of node with " << children.size() << " children" << std::endl;
					for (unsigned int i = 0; i < children.size(); i++) {
						children[i].recordDirection(p, f);
					}
				}
			}
		}

		void refineQuadtree(float total_flux) {
			if (flux.load() / total_flux <= flux_threshold) {
				mergeDNode(); // merge this node childrens' nodes so that it is a leaf now
			} else if (isLeaf()) {
				splitLeafNode();
			}
			
			if (!isLeaf()) {
				if (children.size() != 4) std::cout << "[DEBUG] Tried to record direction of node with " << children.size() << " children" << std::endl;
				for (unsigned int i = 0; i < children.size(); i++) {
					//std::cout << total_flux << " " << flux.load() / total_flux << std::endl;
					children[i].refineQuadtree(total_flux);
				}
			} 
		}

		Point sampleFromQuadTreeNode(Point randomPoint, Sampler &sampler, float &pdf_wo) {
			if(isLeaf() || flux.load() <= 0.f) { // sample random position within the boundaries of the leaf node
				float side = b.b.x - b.a.x;
				pdf_wo *= 1.f / (4.f * M_PI);
				return Point(randomPoint.x * side + b.a.x, randomPoint.y * side + b.a.y);
			} else { //sample child by energy
				const float r = sampler.next1D();
				std::vector<float> cumSum(5, 0.f);
				for (int i = 1; i < 5; i++) {
					cumSum[i] = cumSum[i-1] + children[i-1].flux.load() / flux.load();
					if (cumSum[i] >= r) {
						float alpha = 4.f * (children[i-1].flux.load() / flux.load());
						pdf_wo *= alpha;	
						return children[i-1].sampleFromQuadTreeNode(randomPoint, sampler, pdf_wo);
					}
				}

				// handle last child when floating point imprecision
				float alpha = 4.f * (children[3].flux.load() / flux.load());
				pdf_wo *= alpha;	
				return children[3].sampleFromQuadTreeNode(randomPoint, sampler, pdf_wo);
			}
		}

		void saveLeafNode(int side, std::vector<std::vector<float>> &image) {
			if (isLeaf()) {
				for (unsigned int i = b.a.x * side; i < b.b.x * side; ++i) { 
					for (unsigned int j = b.a.y * side; j < b.b.y * side; ++j) { 
						image[i][j] = flux.load();
					}
				}
			} else {
				if (children.size() != 4) std::cout << "[DEBUG] Tried to save leaf node with " << children.size() << " children" << std::endl;
				for (int i = 0; i < 4; i++) {
					children[i].saveLeafNode(side, image);
				}
			}
		}
};

class QuadTree { // root
	public:
		std::atomic<float> flux{0.f};
		DNode root;
		QuadTree() : root(DNode(0.f, 0, BBox(Point(0.f,0.f), Point(1.f, 1.f)))) {} // initialize the quadtree
		QuadTree(const QuadTree &q) : flux(std::atomic<float>(q.flux.load())), root(q.root) {} // copy the quadtree
		QuadTree(float gui_t) : root(DNode(0.f, 0, BBox(Point(0.f,0.f), Point(1.f, 1.f)))) {flux_threshold = gui_t;} // initialize the quadtree

		QuadTree& operator=(const QuadTree &q) {
			if (this == &q) return *this; // handle self-assignment
			flux.exchange(q.flux.load());
			DNode r = q.root;
			root =(r);
			return *this;
		}

		void splatDirection (glm::vec2 point, glm::vec3 radiance) {
			float irradiance = ((radiance[0] + radiance[1] + radiance[2]) / 3.f); //mean radiance
			flux.store(flux.load() + irradiance);

			root.recordDirection(Point(point.x, point.y), irradiance);
		}

		void refineQuadTree() {
			root.refineQuadtree(flux.load());
		}

		glm::vec2 sampleFromQuadTree(glm::vec2 randomPoint, Sampler &sampler, float &pdf_wo) {
			Point p = root.sampleFromQuadTreeNode(Point(randomPoint.x, randomPoint.y), sampler, pdf_wo);
			return glm::vec2{p.x, p.y};
		}
		
		void reset() {
			if(root.isLeaf()) { 
				flux.store(0.f);
				root.flux.store(0.f);
			} else {
				flux.store(0.f);
				root.mergeDNode();
				root.flux.store(0.f);
				root.level = 0;
				root.b = BBox(Point(0.f, 0.f), Point(1.f,1.f));
			}
			
		}

		void saveQuadtreeImage(int side, int ind, int quadNumber) { // side = 640
			std::string imageName = "images/quadtree_" + std::to_string(ind) + "_" + std::to_string(quadNumber) + ".ppm";
			std::ofstream outfile (imageName, std::ios::out | std::ios::binary);  
			outfile << "P6\n" << side << " " << side << "\n255\n";
			int scale = (int) (1.f / flux_threshold);
			
			std::vector<std::vector<float>> image(side, std::vector<float>(side, 0.f)); 
			root.saveLeafNode(side, std::ref(image));
			for (int i = 0; i < side; ++i) { 
				for (int j = 0; j < side; ++j) { 
					//std::cout << (image[i][j] * 100) / (float) flux << " : " << ind << " : " << flux << std::endl;
					outfile << (unsigned char)(std::min(1.f, (image[i][j] * scale) / (float) flux.load()) * 255) // [0,0.01) --> [0,1) make it visible
						<< (unsigned char)(std::min(1.f, (image[i][j] * scale) / (float) flux.load()) * 255)
						<< (unsigned char)(std::min(1.f, (image[i][j] * scale) / (float) flux.load()) * 255);
				} 
			}			
		 
			outfile.close();
			image.clear();
		}
};

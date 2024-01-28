#pragma once
#include <memory>
#include <atomic>         // std::atomic, std::atomic_flag, ATOMIC_FLAG_INIT

#include "quadtree.h"

int c = 12000;
int currentAxis = 0;

class BBox3D {
	public:
		glm::vec3 a; // upper front left point
		glm::vec3 b; // lower back right point
		float Xlength; 
		float Ylength; 
		float Zlength; 
		BBox3D(glm::vec3 a, glm::vec3 b) : a(a), b(b), Xlength(b.x - a.x), Ylength(b.y - a.y), Zlength(b.z - a.z) {}

		std::vector<BBox3D> splitBBox() {
			currentAxis = (currentAxis + 1) % 3; // alternate axis
			std::vector<BBox3D> bboxes;
			float halfLength = 0.f;

			switch (currentAxis) {
				case 0:
					halfLength = Xlength / 2.f;
					bboxes.push_back(BBox3D(a, glm::vec3{a.x + halfLength, b.y, b.z}));
					bboxes.push_back(BBox3D(glm::vec3{a.x + halfLength, a.y, a.z}, b));
					break;
				case 1:
					halfLength = Ylength / 2.f;
					bboxes.push_back(BBox3D(a, glm::vec3{b.x, a.y + halfLength, b.z}));
					bboxes.push_back(BBox3D(glm::vec3{a.x, a.y + halfLength, a.z}, b));
					break;
				case 2:
					halfLength = Zlength / 2.f;
					bboxes.push_back(BBox3D(a, glm::vec3{b.x, b.y, a.z + halfLength}));
					bboxes.push_back(BBox3D(glm::vec3{a.x, a.y, a.z + halfLength}, b));
					break;
			}

			return bboxes;
		}

		bool containsPoint(glm::vec3 p) {
			return a.x <= p.x && a.y <= p.y && a.z <= p.z && b.x > p.x && b.y > p.y && b.z > p.z;
		}	
};

class SNode {
	public:
		std::vector<SNode> children; // 2 or 0 children inside 1 Node
		std::atomic<float> sampleCount;
		BBox3D b;
		QuadTree previousQuad;
		QuadTree quad;
		SNode(std::atomic<float> sampleCount, BBox3D b, QuadTree previousQuad, QuadTree quad) : sampleCount(std::atomic<float>(sampleCount.load())), b(b), previousQuad(previousQuad), quad(quad) {}
		SNode(const SNode &sn) : sampleCount(std::atomic<float>(sn.sampleCount.load())), b(sn.b), previousQuad(sn.previousQuad), quad(sn.quad) {}


		bool isLeaf() {
			return children.size() == 0;
		}

		void splitLeafNode() {
			std::vector<BBox3D> bboxes = b.splitBBox();
			for (int i = 0; i < 2; i++) {
				QuadTree q = quad;
				QuadTree prevQ = previousQuad;
				SNode node(std::atomic<float>(sampleCount.load() / 2.f), bboxes[i], prevQ, q);
				children.push_back(node);
			}
			quad.reset();
			previousQuad.reset();
		}

		void mergeSNode() {
			if (!isLeaf()) {
				for (int i = 0; i < 2; i++) {
					children[i].mergeSNode(); // applies only if children[i] is not a leaf
				}
				children.clear();
			}
		}

		void refine(int k) { // k is the iteration number :: 2^k
			if (!isLeaf()) {
				children[0].refine(k);
				children[1].refine(k);
			} else if (sampleCount.load() > c * std::sqrt(std::pow(2, k))) {
				splitLeafNode();
			}
		}

		void recordPosition(glm::vec3 point, glm::vec3 wo, std::vector<std::vector<glm::vec3>> &image, int side, glm::vec2 p, glm::vec3 radiance) {
			if (b.containsPoint(point)) {
				sampleCount.store(sampleCount.load() + 1.f);
				if (!isLeaf()) {
					children[0].recordPosition(point, wo, image, side, p, radiance);
					children[1].recordPosition(point, wo, image, side, p, radiance);
				} else {
					// you are in the leaf of the intersection position --> update the corresponding quadtree
					image[p.x * side][p.y * side] = radiance;
					quad.splatDirection(p, radiance);
					quad.refineQuadTree();
				}
			}
		}

		void getQuadTreeDirection(glm::vec3 point, Sampler &sampler, const EnvMap &envmap, std::vector<std::vector<glm::vec3>> &image, int side, glm::vec3 &wo, float &wo_pdf, glm::vec2 &p) {
			if (b.containsPoint(point)) {
				if (!isLeaf()) {
					children[0].getQuadTreeDirection(point, sampler, envmap, image, side, wo, wo_pdf, p);
					children[1].getQuadTreeDirection(point, sampler, envmap, image, side, wo, wo_pdf, p);
				} else {
					glm::vec2 randomPoint = sampler.next2D();
			
					p = previousQuad.sampleFromQuadTree(randomPoint, sampler, wo_pdf);
					wo = sampleSphere(p);
				}
			}	
		}

		void resetQuadTree(int side, int i, int quadNumber) {
			if(isLeaf()) {
				quad.saveQuadtreeImage(side, i, quadNumber);
				QuadTree pq = quad; // !!! Make a deep copy (do not allow shared memory)	
				previousQuad =(pq);
				quad.reset();
			} else {
				children[0].resetQuadTree(side, i, 2 * quadNumber+1);
				children[1].resetQuadTree(side, i, 2 * quadNumber+2);
			}
		}
};

class BinaryTree {
	public:
		std::atomic<float> sampleCount{0.f};
		SNode root;
		BinaryTree(glm::vec3 min_, glm::vec3 max_) : root(SNode(0, BBox3D(min_, max_), QuadTree(), QuadTree())) {}
		BinaryTree(glm::vec3 min_, glm::vec3 max_, int gui_c, float gui_t) : root(SNode(0, BBox3D(min_, max_), QuadTree(gui_t), QuadTree())) {c = gui_c;}

		void splatPosition (glm::vec3 point, glm::vec3 wo, std::vector<std::vector<glm::vec3>> &image, int side, glm::vec2 p, glm::vec3 radiance) {
			sampleCount.store(sampleCount.load() + 1.f);
			root.recordPosition(point, wo, image, side, p, radiance);
		}

		void refine(int k) {
			root.refine(k);
		}

		void getQuadTreeDirection(glm::vec3 intersectionPoint, Sampler &sampler, const EnvMap &envmap, std::vector<std::vector<glm::vec3>> &image, int side, glm::vec3 &wo, float &wo_pdf, glm::vec2 &p) {
			root.getQuadTreeDirection(intersectionPoint, sampler, envmap, image, side, wo, wo_pdf, p);
		}

		void resetQuadTree(int side, int i) {
			root.resetQuadTree(side, i, 0);
		}

		void reset() {
			if(root.isLeaf()) { 
				sampleCount.store(0.f);
				root.sampleCount.store(0.f);
				root.b = BBox3D(glm::vec3(0.f), glm::vec3(1.f));
				root.quad.reset();
				root.previousQuad.reset();
			} else {
				sampleCount = 0.f;
				root.mergeSNode();
				root.sampleCount.store(0.f);
				root.b = BBox3D(glm::vec3(0.f), glm::vec3(1.f));
				root.quad.reset();
				root.previousQuad.reset();
			}
			
		}
};

#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // std::replace

#include "tinyparser-mitsuba.h"

#include <glm/glm.hpp>
using namespace glm;
using namespace std;

template<class IndexSize>
bool parseObject(const char *path, vector<vec3> &vertices, vector<vec2> &uvs, vector< vec3> &normals,
	vector<IndexSize> &vertex_indices, vector<IndexSize> &uv_indices, vector<IndexSize> &normal_indices,
	vector<vec3> &ordered_vertices)
{
	const auto size = vertices.size();
	std::string line;
	std::string type;
	std::ifstream file(path);

	if (!file) return false;
	while (std::getline(file, line))
	{
		type.clear();
		std::replace(line.begin(), line.end(), '/', ' ');
		std::istringstream ss(std::move(line));
		ss >> type;
		if (type == "v")
		{
			vec3 vertex(0.f);
			ss >> vertex[0] >> vertex[1] >> vertex[2];
			vertices.push_back(vertex);
		}
		else if (type == "vt")
		{
			vec2 uv(0.f);
			ss >> uv[0] >> uv[1];
			uvs.push_back(uv);
		}
		else if (type == "vn")
		{
			vec3 normal(0.f);
			ss >> normal[0] >> normal[1] >> normal[2];
			normals.push_back(normal);
		}
		else if (type == "f")
		{
			const auto line = ss.str();
			if (std::count(line.begin(), line.end(), ' ') == 9)
			{
				int vertexIndex[3], uvIndex[3], normalIndex[3];
				ss >> vertexIndex[0] >> uvIndex[0] >> normalIndex[0] 
					>> vertexIndex[1] >> uvIndex[1] >> normalIndex[1]
					>> vertexIndex[2] >> uvIndex[2] >> normalIndex[2];

				// if (ss.rdbuf()->in_avail() != 0)
				// {
				// 	printf("File can't be read by this parser : vertex, uv, normal \n");
				// 	return false;
				// }

				// get the indices of the triangles from where to take the values of the temp vectors
				vertex_indices.push_back(size + vertexIndex[0]-1);
				vertex_indices.push_back(size + vertexIndex[1]-1);
				vertex_indices.push_back(size + vertexIndex[2]-1);
				uv_indices    .push_back(size + uvIndex[0]-1);
				uv_indices    .push_back(size + uvIndex[1]-1);
				uv_indices    .push_back(size + uvIndex[2]-1);
				normal_indices.push_back(size + normalIndex[0]-1);
				normal_indices.push_back(size + normalIndex[1]-1);
				normal_indices.push_back(size + normalIndex[2]-1);

				ordered_vertices.push_back(vertices[vertexIndex[0]]);
				ordered_vertices.push_back(vertices[vertexIndex[1]]);
				ordered_vertices.push_back(vertices[vertexIndex[2]]);
			}
			else throw std::runtime_error("can't handle this representation yet: " + line);
		}
	}
	file.close();
	return true;
}

template<class IndexSize>
bool parseFile(const std::string &dir_path, const std::string &scene_name, vector<vec3> &vertices, vector<vec2> &uvs, vector< vec3> &normals,
	vector<IndexSize> &vertex_indices, vector<IndexSize> &uv_indices, vector<IndexSize> &normal_indices, vector<vec3> &ordered_vertices)
	{
		std::string path(dir_path + scene_name + ".xml");
		tinyparser_mitsuba::SceneLoader loader;
		tinyparser_mitsuba::Scene scene = loader.loadFromFile(path.c_str());
		// std::cout << "properties:\n\n";
		// for (const auto & prop : scene.properties()) std::cout << prop.first << ", type: " << prop.second.type() << "\n";
		// std::cout << "anonymous children:\n\n";
		for (const auto & ac : scene.anonymousChildren()) 
		{
			if (ac->type() == 10)//shape
			{
				// std::cout << "   plugin type: " << ac->pluginType() << ", id: " << ac->id() << "\n";
				// std::cout << "   properties:\n\n";
				// for (const auto & prop : ac->properties()) 
				// {
				// 	if (prop.first == "filename" && prop.second.type() == 8) std::cout << "   " << prop.first << ", value: " << prop.second.getString() << "\n";
				// 	else std::cout << "   " << prop.first << ", type: " << prop.second.type() << "\n";
				// }
				// std::cout << "   anonymous children:\n\n";
				// for (const auto & ac2 : ac->anonymousChildren()) std::cout << "   type: " << ac2->type() << ", plugin type: " << ac2->pluginType() << ", id: " << ac2->id() << "\n";	
				// std::cout << "   named children:\n\n";
				// for (const auto & nc : ac->namedChildren()) std::cout << "   " << nc.first << ", type: " << "\n";
				for (const auto & prop : ac->properties()) 
				{
					if (prop.first == "filename" && prop.second.type() == 8) 
					{
						path = dir_path + prop.second.getString();
						parseObject(path.c_str(), vertices, uvs, normals, vertex_indices, uv_indices, normal_indices, ordered_vertices);
						break;
					}
				}
			} 
			// else if (ac->type() == 1)//bsdf
			// {
			// 	std::cout << "   plugin type: " << ac->pluginType() << ", id: " << ac->id() << "\n";
			// 	std::cout << "   properties:\n\n";
			// 	for (const auto & prop : ac->properties()) std::cout << "   " << prop.first << ", type: " << prop.second.type() << "\n";
			// 	std::cout << "   anonymous children:\n\n";
			// 	for (const auto & ac2 : ac->anonymousChildren()) 
			// 	{
			// 		std::cout << "      type: " << ac2->type() << ", plugin type: " << ac2->pluginType() << ", id: " << ac2->id() << "\n";	
			// 		std::cout << "      properties:\n\n";
			// 		for (const auto & prop : ac2->properties()) std::cout << "      " << prop.first << ", type: " << prop.second.type() << "\n";
			// 		std::cout << "      anonymous children:\n\n";
			// 		for (const auto & ac3 : ac2->anonymousChildren()) 
			// 		{
			// 			std::cout << "      type: " << ac3->type() << ", plugin type: " << ac3->pluginType() << ", id: " << ac3->id() << "\n";	
			// 		}
			// 		std::cout << "      named children:\n\n";
			// 		for (const auto & nc : ac2->namedChildren()) std::cout << "      " << nc.first << ", type: " << "\n";
			// 	}
			// 	std::cout << "   named children:\n\n";
			// 	for (const auto & nc : ac->namedChildren()) std::cout << "   " << nc.first << ", type: " << "\n";
			// }
		}
		// std::cout << "named children:\n\n";
		// for (const auto & nc : scene.namedChildren()) std::cout << nc.first << ", type: " << "\n";

		return true;
	}
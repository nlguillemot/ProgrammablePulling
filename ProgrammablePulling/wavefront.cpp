/*
 * wavefront.cpp
 *
 *  Created on: Jan 24, 2010
 *      Author: aqnuep
 *
 *  Note: Do not use this module in any product as it is a very rough
 *        wavefront object loader made only for the demo program and
 *        is far from product quality
 */

#include "wavefront.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <tuple>

namespace demo {

WaveFrontObj::WaveFrontObj(const char* filename)
{
    struct IndexGroup
    {
        GLuint i, v, n;
    
        bool operator==(const IndexGroup& ig) const {
            return std::tie(v, ig.v) == std::tie(n, ig.n);
        }
        
        bool operator<(const IndexGroup& ig) const {
            return std::tie(v,n) < std::tie(ig.v, ig.n);
        }
    };

	std::ifstream file(filename);
	if (!file) {
		std::cerr << "Unable to open file: " << filename << std::endl;
		return;
	}

    std::vector<glm::vec3> unmerged_positions;
    std::vector<glm::vec3> unmerged_normals;
    std::set<IndexGroup> indexCache;

    GLuint id = 0;

    std::string line;
    while (std::getline(file, line))
    {
    	if (line.size() >= 2 && line[0] == 'v' && line[1] == ' ') {
            glm::vec3 v;
            sscanf(line.c_str(), "v %f %f %f", &v.x, &v.y, &v.z);
    		unmerged_positions.push_back(v);
    	} 
        else if (line.size() >= 2 && line[0] == 'v' && line[1] == 'n') {
            glm::vec3 vn;
            sscanf(line.c_str(), "vn %f %f %f", &vn.x, &vn.y, &vn.z);
    		unmerged_normals.push_back(vn);
    	} 
        else if (line.size() >= 1 && line[0] == 'f') {
            int ivs[3];
            int ivns[3];
            
            sscanf(line.c_str(), "f %d//%d %d//%d %d//%d", &ivs[0], &ivns[0], &ivs[1], &ivns[1], &ivs[2], &ivns[2]);
            
            // some obj files have negative indices, which need to be handled specially.
            assert(ivs[0] >= 1 && ivns[0] >= 1 && ivs[1] >= 1 && ivns[1] >= 1 && ivs[2] >= 1 && ivns[2] >= 1);

            for (int i = 0; i < 3; i++) 
            {
    			IndexGroup ig;
    			ig.v = ivs[i];
                ig.n = ivns[i];
                ig.i = id;

                std::set<IndexGroup>::iterator it;
                bool inserted;
                std::tie(it, inserted) = indexCache.insert(ig);
    			if (inserted) {
                    Positions.push_back(unmerged_positions[ig.v - 1]);
                    Normals.push_back(unmerged_normals[ig.n - 1]);
					Indices.push_back(id);
                    id += 1;
    			} else {
    				Indices.push_back(it->i);
    			}
    		}
    	}
    }
}

void WaveFrontObj::dump() {
    for (const glm::vec3& v : Positions) {
		std::cout << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}

    for (const glm::vec3& n : Normals) {
		std::cout << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
	}

    for (size_t ii = 0; ii < Indices.size(); ii += 3) {
		std::cout << "f " << Indices[ii * 3 + 0];
        std::cout << " "  << Indices[ii * 3 + 1];
		std::cout << " "  << Indices[ii * 3 + 2] << std::endl;
	}
}

} /* namespace demo */

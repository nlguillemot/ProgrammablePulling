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

    printf("Loading mesh: %s\n", filename);

    std::vector<glm::vec3> unmerged_positions;
    std::vector<glm::vec3> unmerged_normals;
    std::set<IndexGroup> indexCache;

    struct uniquevec3 {
        glm::vec3 v;
        uint32_t i;

        bool operator<(const uniquevec3& o) const {
            return std::make_tuple(v.x, v.y, v.z) < std::make_tuple(o.v.x, o.v.y, o.v.z);
        }
    };
    std::set<uniquevec3> unique_positions;
    std::set<uniquevec3> unique_normals;

    GLuint id = 0;

    enum OBJFormat
    {
        OBJFormat_Unknown,
        OBJFormat_V_N,
        OBJFormat_VTN
    };

    OBJFormat format = OBJFormat_Unknown;

    std::string line;
    while (std::getline(file, line))
    {
    	if (line.size() >= 2 && line[0] == 'v' && line[1] == ' ') {
            glm::vec3 v;
            sscanf(line.c_str(), "v %f %f %f", &v.x, &v.y, &v.z);
    		unmerged_positions.push_back(v);

            uniquevec3 u = { v, (uint32_t)UniquePositions.size() };
            if (unique_positions.insert(u).second)
            {
                UniquePositions.push_back(v);
            }
    	} 
        else if (line.size() >= 2 && line[0] == 'v' && line[1] == 'n') {
            glm::vec3 vn;
            sscanf(line.c_str(), "vn %f %f %f", &vn.x, &vn.y, &vn.z);
    		unmerged_normals.push_back(vn);

            uniquevec3 u = { vn, (uint32_t)UniqueNormals.size() };
            if (unique_normals.insert(u).second)
            {
                UniqueNormals.push_back(vn);
            }
    	} 
        else if (line.size() >= 1 && line[0] == 'f') {
            int ivs[4] = { 0, 0, 0, 0 };
            int ivns[4] = { 0, 0, 0, 0 };
            int ivts[4] = { 0, 0, 0, 0 };
            
            if (format == OBJFormat_Unknown || format == OBJFormat_V_N)
            {
                if (sscanf(line.c_str(), "f %d//%d %d//%d %d//%d %d//%d", &ivs[0], &ivns[0], &ivs[1], &ivns[1], &ivs[2], &ivns[2], &ivs[3], &ivns[3]) >= 6)
                {
                    format = OBJFormat_V_N;
                }
            }
            
            if (format == OBJFormat_Unknown || format == OBJFormat_VTN)
            {
                if (sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &ivs[0], &ivts[0], &ivns[0], &ivs[1], &ivts[1], &ivns[1], &ivs[2], &ivts[2], &ivns[2], &ivs[3], &ivts[3], &ivns[3]) >= 9)
                {
                    format = OBJFormat_VTN;
                }
            }
            
            const int triangulation[] = { 0, 1, 2, 0, 2, 3 };
            int numVerts = ivs[3] == 0 ? 3 : 6; // tri vs quad
            for (int i = 0; i < numVerts; i++) 
            {
    			IndexGroup ig;
    			ig.v = ivs[triangulation[i]];
                ig.n = ivns[triangulation[i]];
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

                PositionIndices.push_back(unique_positions.find(uniquevec3{unmerged_positions[ig.v - 1], 0})->i);
                NormalIndices.push_back(unique_normals.find(uniquevec3{unmerged_normals[ig.n - 1], 0})->i);
    		}
    	}
    }

    printf("unique positions: %zu\n", unique_positions.size());
    printf("unique normals: %zu\n", unique_normals.size());
}

} /* namespace demo */

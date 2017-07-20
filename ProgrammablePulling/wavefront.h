/*
 * wavefront.h
 *
 *  Created on: Jan 24, 2010
 *      Author: aqnuep
 *
 *  Note: Do not use this module in any product as it is a very rough
 *        wavefront object loader made only for the demo program and
 *        is far from product quality
 */

#ifndef WAVEFRONT_H_
#define WAVEFRONT_H_

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace demo {

class WaveFrontObj 
{
public:
	explicit WaveFrontObj(const char* filename);

    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::uint> Indices;

    std::vector<glm::vec3> UniquePositions;
    std::vector<glm::vec3> UniqueNormals;
    std::vector<glm::uint> PositionIndices;
    std::vector<glm::uint> NormalIndices;
};

} /* namespace demo */

#endif /* WAVEFRONT_H_ */

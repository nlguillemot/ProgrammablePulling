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
	WaveFrontObj(const char* filename);
	void dump();

    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::uint> Indices;
};

} /* namespace demo */

#endif /* WAVEFRONT_H_ */

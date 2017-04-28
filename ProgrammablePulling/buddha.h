/*
 * buddha.h
 *
 *  Created on: Sep 24, 2011
 *      Author: aqnuep
 */

#ifndef BUDDHA_H_
#define BUDDHA_H_

#include <memory>

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768

namespace buddha {

enum VertexPullingMode
{
    FIXED_FUNCTION_AOS_MODE,     // fixed-function vertex pulling
    FIXED_FUNCTION_SOA_MODE,     // fixed-function vertex pulling
    FETCHER_AOS_MODE,            // programmable attribute fetching
    FETCHER_SOA_MODE,            // programmable attribute fetching
    FETCHER_IMAGE_AOS_MODE,  // programmable attribute fetching from an image instead of a texture in array of structures format
    FETCHER_IMAGE_SOA_MODE,  // programmable attribute fetching from an image instead of a texture in structure of arrays format
    PULLER_AOS_MODE,             // fully programmable vertex pulling
    PULLER_SOA_MODE,             // fully programmable vertex pulling
    NUMBER_OF_MODES
};

class IBuddhaDemo
{
public:
    static std::shared_ptr<IBuddhaDemo> Create();

    virtual void renderScene(float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds) = 0;
};

} /* namespace buddha */

#endif /* BUDDHA_H_ */

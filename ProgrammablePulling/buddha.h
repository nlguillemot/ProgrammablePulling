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
    // fixed-function vertex pulling
    FIXED_FUNCTION_AOS_MODE,
    FIXED_FUNCTION_SOA_MODE,
    FIXED_FUNCTION_AOS_QUANTIZED_MODE,
    FIXED_FUNCTION_SOA_QUANTIZED_MODE,
    // fetch attributes using gl_VertexID from index buffer
    FETCHER_AOS_MODE,
    FETCHER_SOA_MODE,
    FETCHER_AOS_QUANTIZED_MODE,
    FETCHER_SOA_QUANTIZED_MODE,
    FETCHER_IMAGE_AOS_MODE,
    FETCHER_IMAGE_SOA_MODE,
    FETCHER_IMAGE_AOS_QUANTIZED_MODE,
    FETCHER_IMAGE_SOA_QUANTIZED_MODE,
    // read index buffer with gl_VertexID, then fetch attributes with result
    PULLER_AOS_MODE,
    PULLER_SOA_MODE,
    PULLER_AOS_QUANTIZED_MODE,
    PULLER_SOA_QUANTIZED_MODE,
    PULLER_IMAGE_AOS_MODE,
    PULLER_IMAGE_SOA_MODE,
    PULLER_IMAGE_AOS_QUANTIZED_MODE,
    PULLER_IMAGE_SOA_QUANTIZED_MODE,
    //
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

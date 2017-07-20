/*
 * buddha.h
 *
 *  Created on: Sep 24, 2011
 *      Author: aqnuep
 */

#ifndef BUDDHA_H_
#define BUDDHA_H_

#include <glm/glm.hpp>

#include <memory>

#define DEFAULT_SCREEN_WIDTH         1024
#define DEFAULT_SCREEN_HEIGHT        768

namespace buddha {

enum VertexPullingMode
{
    // fixed-function vertex pulling
    FIXED_FUNCTION_AOS_MODE,
    FIXED_FUNCTION_AOS_XYZW_MODE,
    FIXED_FUNCTION_SOA_MODE,
    FIXED_FUNCTION_INTERLEAVED_MODE,
    // fetch attributes using gl_VertexID from index buffer
    FETCHER_AOS_1RGBAFETCH_MODE,
    FETCHER_AOS_1RGBFETCH_MODE,
    FETCHER_AOS_3FETCH_MODE,
    FETCHER_SOA_MODE,
    FETCHER_IMAGE_AOS_1FETCH_MODE,
    FETCHER_IMAGE_AOS_3FETCH_MODE,
    FETCHER_IMAGE_SOA_MODE,
    FETCHER_SSBO_AOS_1FETCH_MODE,
    FETCHER_SSBO_AOS_3FETCH_MODE,
    FETCHER_SSBO_SOA_MODE,
    // read index buffer with gl_VertexID, then fetch attributes with result
    PULLER_AOS_1RGBAFETCH_MODE,
    PULLER_AOS_1RGBFETCH_MODE,
    PULLER_AOS_3FETCH_MODE,
    PULLER_SOA_MODE,
    PULLER_IMAGE_AOS_1FETCH_MODE,
    PULLER_IMAGE_AOS_3FETCH_MODE,
    PULLER_IMAGE_SOA_MODE,
    PULLER_SSBO_AOS_1FETCH_MODE,
    PULLER_SSBO_AOS_3FETCH_MODE,
    PULLER_SSBO_SOA_MODE,
    //
    NUMBER_OF_MODES
};

class IBuddhaDemo
{
public:
    static std::shared_ptr<IBuddhaDemo> Create();

    virtual int addMesh(const char* path) = 0;

    virtual void renderScene(int meshID, const glm::mat4& modelMatrix, int screenWidth, int screenHeight, float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds) = 0;
};

} /* namespace buddha */

#endif /* BUDDHA_H_ */

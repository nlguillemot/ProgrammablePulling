layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

struct CacheEntry
{
    uint PositionIndex;
    uint NormalIndex;
    int Address;
    uint _padding;
};

// collision resolution strategy = separate chaining
struct CacheBucket
{
    CacheEntry entries[NUM_CACHE_ENTRIES_PER_BUCKET];
};

// Keep this in sync with VERTEX_CACHE_ENTRY_SIZE_IN_DWORDS
struct CachedVertex
{
    vec4 m_outVertexPosition;
    vec4 m_outVertexNormal;
    vec4 m_gl_Position;
};

layout(std430, binding = 0) restrict readonly buffer PositionIndexBuffer { uint PositionIndices[]; };
layout(std430, binding = 1) restrict readonly buffer NormalIndexBuffer { uint NormalIndices[]; };
layout(std430, binding = 2) restrict readonly buffer PositionBuffer { vec4 Positions[]; };
layout(std430, binding = 3) restrict readonly buffer NormalBuffer{ vec4 Normals[]; };

layout(std430, binding = 4) coherent volatile restrict buffer VertexCacheCounterBuffer { uint VertexCacheCounter; };
layout(std430, binding = 5) coherent volatile restrict buffer VertexCacheBuffer { CachedVertex VertexCache[]; };
layout(std430, binding = 6) coherent volatile restrict buffer CacheBucketsBuffer { CacheBucket CacheBuckets[]; };
layout(std430, binding = 7) coherent volatile restrict buffer CacheBucketLocksBuffer { uint CacheBucketLocks[]; };

#ifdef ENABLE_CACHE_MISS_COUNTER
layout(std430, binding = 8) restrict buffer CacheMissCountBuffer { uint CacheMissCounter; };
#endif

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex{
    vec4 gl_Position;
};

CachedVertex recompute_vertex(uint positionIndex, uint normalIndex);

CachedVertex lookup_vertex_cache(int hashID, uint positionIndex, uint normalIndex)
{
    bool should_store = false;

    for (int readAttempt = 0; readAttempt < NUM_READ_CACHE_LOCK_ATTEMPTS; readAttempt++)
    {
        // Try to acquire a shared reader lock
        if (atomicAdd(CacheBucketLocks[hashID], 1) < MAX_SIMULTANEOUS_READERS)
        {
            // Acquired read access, see if the entry we want is there.
            int address = -1;
            for (int e = 0; e < NUM_CACHE_ENTRIES_PER_BUCKET; e++)
            {
                if (CacheBuckets[hashID].entries[e].PositionIndex == positionIndex &&
                    CacheBuckets[hashID].entries[e].NormalIndex == normalIndex)
                {
                    address = CacheBuckets[hashID].entries[e].Address;
                    break;
                }
            }

            // Release the reader lock
            atomicAdd(CacheBucketLocks[hashID], -1);

            if (address >= 0)
            {
                // found a matching entry, so grab the vertex from the cache, and we're done.
                return VertexCache[address];
            }
            else
            {
                // didn't find a matching entry, so will try to write into it.
                should_store = true;
            }

            break;
        }
    }

    // couldn't find the entry in the cache, so we try to produce it ourselves to write it in.
    CachedVertex vertex = recompute_vertex(positionIndex, normalIndex);

    if (should_store)
    {
        for (int writeAttempt = 0; writeAttempt < NUM_WRITE_CACHE_LOCK_ATTEMPTS; writeAttempt++)
        {
            // Try to acquire write ownership by saturating all possible reader slots
            if (atomicCompSwap(CacheBucketLocks[hashID], 0, MAX_SIMULTANEOUS_READERS) == 0)
            {
                // Acquired the write lock, so allocate the vertex and put it in.
                int newAddress = int(atomicAdd(VertexCacheCounter, 1));
                VertexCache[newAddress] = vertex;

                CacheEntry newEntry;
                newEntry.PositionIndex = positionIndex;
                newEntry.NormalIndex = normalIndex;
                newEntry.Address = newAddress;

                // push the cache entry into the FIFO
                for (int fifoIdx = NUM_CACHE_ENTRIES_PER_BUCKET - 1; fifoIdx > 0; fifoIdx--)
                {
                    CacheBuckets[hashID].entries[fifoIdx] = CacheBuckets[hashID].entries[fifoIdx - 1];
                }

                CacheBuckets[hashID].entries[0] = newEntry;

                // Release the write lock
                CacheBucketLocks[hashID] = 0;
                break;
            }
        }
    }

    // If we reach this point, we couldn't secure read access to the cache.
    // Therefore, recreate the vertex redundantly.
    return vertex;
}

void main()
{
    /* fetch index from storage buffer */
    uint positionIndex = PositionIndices[gl_VertexID];
    uint normalIndex = NormalIndices[gl_VertexID];

    int hashID = int(positionIndex + 33 * normalIndex) & (NUM_CACHE_BUCKETS - 1);
   
    CachedVertex vertex = lookup_vertex_cache(hashID, positionIndex, normalIndex);

    outVertexPosition = vertex.m_outVertexPosition.xyz;
    outVertexNormal = vertex.m_outVertexNormal.xyz;
    gl_Position = vertex.m_gl_Position;
}

CachedVertex recompute_vertex(uint positionIndex, uint normalIndex)
{
#ifdef ENABLE_CACHE_MISS_COUNTER
    atomicAdd(CacheMissCounter, 1);
#endif

    /* fetch attributes from storage buffer */
    vec3 inVertexPosition;
    inVertexPosition.xyz = Positions[positionIndex].xyz;

    vec3 inVertexNormal;
    inVertexNormal.xyz = Normals[normalIndex].xyz;

    /* transform vertex and normal */
    CachedVertex vertex;
    vertex.m_outVertexPosition = Transform.ModelViewMatrix * vec4(inVertexPosition, 1);
    vertex.m_outVertexNormal = vec4(mat3(Transform.ModelViewMatrix) * inVertexNormal, 0);
    vertex.m_gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);

    return vertex;
}

layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

// Keep this in sync with VERTEX_CACHE_ENTRY_SIZE_IN_DWORDS
struct CachedVertex
{
    uint PositionIndex;
    uint NormalIndex;
    
    uint _padding[2];

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
layout(std430, binding = 6) coherent volatile restrict buffer CacheBucketsBuffer { uvec4 CacheBuckets[]; };
layout(std430, binding = 7) coherent volatile restrict buffer CacheBucketLocksBuffer { uint CacheBucketLocks[]; };

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex{
    vec4 gl_Position;
};

int get_address_from_bucket(int id, uvec4 bucket)
{
    if (bucket.x == id)
    {
        return int(bucket.y);
    }
    else if (bucket.z == id)
    {
        return int(bucket.w);
    }
    else
    {
        return -1;
    }
}

// From "Deferred Attribute Interpolation Shading" (Listing 3.1) in GPU Pro 7
// Authors: Christoph Schied and Carsten Dachsbacher
void lookup_memoization_cache(int id, int id_mask, int store_size, out int address, out bool should_store)
{
    should_store = false;
    int hash = id & id_mask;
    uvec4 b = CacheBuckets[hash];
    address = get_address_from_bucket(id, b);
    for (int k = 0; address < 0 && k < NUM_CACHE_LOCK_ATTEMPTS; k++)
    {
        // ID not found in cache, make several attempts.
        uint lock = atomicExchange(CacheBucketLocks[hash], CACHE_BUCKET_LOCKED);
        if (lock == CACHE_BUCKET_UNLOCKED)
        {
            // Gained exclusive access to the bucket.
            b = CacheBuckets[hash];
            address = get_address_from_bucket(id, b);
            if (address < 0)
            {
                // Allocate new storage
                address = int(atomicAdd(VertexCacheCounter, store_size));
                b.zw = b.xy; // Update bucket FIFO.
                b.xy = uvec2(id, address);
                CacheBuckets[hash] = b;
                should_store = true;
            }
            
            CacheBucketLocks[hash] = CACHE_BUCKET_UNLOCKED;
        }

        // Use if(expr){} if(!expr){} construct to explicitly sequence the branches
        if (lock == CACHE_BUCKET_LOCKED)
        {
            for (int i = 0; i < NUM_CACHE_LOCK_PUSH_THROUGH_ATTEMPTS && lock == CACHE_BUCKET_LOCKED; i++)
            {
                lock = CacheBucketLocks[hash].r;
            }
            b = CacheBuckets[hash];
            address = get_address_from_bucket(id, b);
        }
    }

    if (address < 0)
    {
        // Cache lookup failed, store redundantly.
        address = int(atomicAdd(VertexCacheCounter, store_size));
        should_store = true;
    }
}

void main()
{
    /* fetch index from storage buffer */
    uint positionIndex = PositionIndices[gl_VertexID];
    uint normalIndex = NormalIndices[gl_VertexID];

    uint id = positionIndex + 33 * normalIndex;
   
    int address;
    bool should_store;
    lookup_memoization_cache(int(id), NUM_CACHE_BUCKETS - 1, 1, address, should_store);
    if (!should_store)
    {
        if (VertexCache[address].PositionIndex == positionIndex && VertexCache[address].NormalIndex == normalIndex)
        {
            outVertexPosition = VertexCache[address].m_outVertexPosition.xyz;
            outVertexNormal = VertexCache[address].m_outVertexNormal.xyz;
            gl_Position = VertexCache[address].m_gl_Position;
            return;
        }
    }

    /* fetch attributes from storage buffer */
    vec3 inVertexPosition;
    inVertexPosition.xyz = Positions[positionIndex].xyz;

    vec3 inVertexNormal;
    inVertexNormal.xyz = Normals[normalIndex].xyz;

    /* transform vertex and normal */
    outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
    outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
    gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);

    if (should_store)
    {
        VertexCache[address].PositionIndex = positionIndex;
        VertexCache[address].NormalIndex = normalIndex;
        VertexCache[address].m_outVertexPosition = vec4(outVertexPosition, 1);
        VertexCache[address].m_outVertexNormal = vec4(outVertexNormal, 0);
        VertexCache[address].m_gl_Position = gl_Position;
    }
}

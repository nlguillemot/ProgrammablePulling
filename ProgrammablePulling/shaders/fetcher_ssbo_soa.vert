#version 430 core

layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

layout(std430, binding = 0) restrict readonly buffer PositionXBuffer { float PositionXs[]; };
layout(std430, binding = 1) restrict readonly buffer PositionYBuffer { float PositionYs[]; };
layout(std430, binding = 2) restrict readonly buffer PositionZBuffer { float PositionZs[]; };
layout(std430, binding = 3) restrict readonly buffer NormalXBuffer { float NormalXs[]; };
layout(std430, binding = 4) restrict readonly buffer NormalYBuffer { float NormalYs[]; };
layout(std430, binding = 5) restrict readonly buffer NormalZBuffer { float NormalZs[]; };

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex{
    vec4 gl_Position;
};

void main(void) {

    /* fetch attributes from storage buffer */
    vec3 inVertexPosition;
    inVertexPosition.x = PositionXs[gl_VertexID];
    inVertexPosition.y = PositionYs[gl_VertexID];
    inVertexPosition.z = PositionZs[gl_VertexID];

    vec3 inVertexNormal;
    inVertexNormal.x = NormalXs[gl_VertexID];
    inVertexNormal.y = NormalYs[gl_VertexID];
    inVertexNormal.z = NormalZs[gl_VertexID];

    /* transform vertex and normal */
    outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
    outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
    gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);

}

#version 430 core

layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

layout(std430, binding = 0) restrict readonly buffer PositionBuffer { vec4 Positions[]; };
layout(std430, binding = 1) restrict readonly buffer NormalBuffer { vec4 Normals[]; };

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex{
    vec4 gl_Position;
};

void main(void) {

    /* fetch attributes from storage buffer */
    vec3 inVertexPosition;
    inVertexPosition.xyz = Positions[gl_VertexID].xyz;

    vec3 inVertexNormal;
    inVertexNormal.xyz = Normals[gl_VertexID].xyz;

    /* transform vertex and normal */
    outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
    outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
    gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);

}

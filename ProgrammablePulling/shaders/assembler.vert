layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

layout(std430, binding = 0) restrict readonly buffer PositionBuffer { vec4 Positions[]; };
layout(std430, binding = 1) restrict readonly buffer NormalBuffer{ vec4 Normals[]; };

out vec3 assembly;

void main()
{
    if ((gl_VertexID & 0x80000000) == 0) // process position
    {
        /* fetch attributes from storage buffer */
        vec3 inVertexPosition;
        inVertexPosition.xyz = Positions[gl_VertexID & 0x7FFFFFFF].xyz;

        /* transform vertex */
        assembly = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
    }
    else // process normal
    {
        /* fetch attributes from storage buffer */
        vec3 inVertexNormal;
        inVertexNormal.xyz = Normals[gl_VertexID & 0x7FFFFFFF].xyz;

        /* transform normal */
        assembly = mat3(Transform.ModelViewMatrix) * inVertexNormal;
    }
}

layout(triangles_adjacency) in; // hack to pass in a patch of 6 inputs to the GS
layout(triangle_strip, max_vertices = 3) out;

layout(std140, binding = 0) uniform transform {
    mat4 ModelViewMatrix;
    mat4 ProjectionMatrix;
    mat4 MVPMatrix;
} Transform;

in vec3 assembly[];

out vec3 outVertexPosition;
out vec3 outVertexNormal;

void main()
{
    for (int i = 0; i < 3; i++)
    {
        outVertexPosition = assembly[i * 2 + 0];
        outVertexNormal = assembly[i * 2 + 1];
        gl_Position = Transform.ProjectionMatrix * vec4(assembly[i * 2 + 0].xyz, 1);
        EmitVertex();
    }

    EndPrimitive();
}

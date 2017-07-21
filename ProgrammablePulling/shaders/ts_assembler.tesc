layout(vertices = 6) out;

in vec3 assembly[];

patch out vec3 PatchAssembly[6];

void main()
{
    PatchAssembly[gl_InvocationID] = assembly[gl_InvocationID];
}

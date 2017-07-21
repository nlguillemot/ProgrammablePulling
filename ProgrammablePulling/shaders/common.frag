in vec3 outVertexPosition;
in vec3 outVertexNormal;

layout(location = 0) out vec4 outColor;

void main(void)
{
	// lighting in view space
	vec3 L = normalize(vec3(0) - outVertexPosition);
	vec3 N = normalize(outVertexNormal);

	outColor = vec4(N * max(0, dot(N, L)), 1.0);
}

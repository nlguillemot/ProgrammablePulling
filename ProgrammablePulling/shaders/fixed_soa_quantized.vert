#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(location = 0) in float inPositionX;
layout(location = 1) in float inPositionY;
layout(location = 2) in float inPositionZ;
layout(location = 3) in float inNormalXQuantized;
layout(location = 4) in float inNormalYQuantized;
layout(location = 5) in float inNormalZQuantized;

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void)
{
	// manually do SNORM conversion since I don't think VAOs can do that.
	float inNormalX = inNormalXQuantized * 2.0 - 1.0;
	float inNormalY = inNormalYQuantized * 2.0 - 1.0;
	float inNormalZ = inNormalZQuantized * 2.0 - 1.0;

	vec3 inVertexPosition = vec3(inPositionX, inPositionY, inPositionZ);
	vec3 inVertexNormal = vec3(inNormalX, inNormalY, inNormalZ);

	/* transform vertex and normal */
	outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

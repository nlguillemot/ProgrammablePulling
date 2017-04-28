#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(binding = 0) uniform isamplerBuffer indexBuffer;
layout(binding = 1) uniform samplerBuffer positionBuffer;
layout(binding = 2) uniform samplerBuffer normalBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */
	int inIndex = texelFetch(indexBuffer, gl_VertexID).x;

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	inVertexPosition.x = texelFetch(positionBuffer, inIndex * 3 + 0).x; 
	inVertexPosition.y = texelFetch(positionBuffer, inIndex * 3 + 1).x; 
	inVertexPosition.z = texelFetch(positionBuffer, inIndex * 3 + 2).x; 
	
	vec3 inVertexNormal;
	inVertexNormal.x   = texelFetch(normalBuffer, inIndex * 3 + 0).x; 
	inVertexNormal.y   = texelFetch(normalBuffer, inIndex * 3 + 1).x; 
	inVertexNormal.z   = texelFetch(normalBuffer, inIndex * 3 + 2).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

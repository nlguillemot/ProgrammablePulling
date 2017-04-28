#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(binding = 0) uniform isamplerBuffer indexBuffer;
layout(binding = 1) uniform samplerBuffer attribBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */
	int inIndex = texelFetch(indexBuffer, gl_VertexID).x;

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	vec3 inVertexNormal;
	inVertexPosition.x = texelFetch(attribBuffer, inIndex * 6 + 0).x; 
	inVertexPosition.y = texelFetch(attribBuffer, inIndex * 6 + 1).x; 
	inVertexPosition.z = texelFetch(attribBuffer, inIndex * 6 + 2).x; 
	inVertexNormal.x   = texelFetch(attribBuffer, inIndex * 6 + 3).x; 
	inVertexNormal.y   = texelFetch(attribBuffer, inIndex * 6 + 4).x; 
	inVertexNormal.z   = texelFetch(attribBuffer, inIndex * 6 + 5).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

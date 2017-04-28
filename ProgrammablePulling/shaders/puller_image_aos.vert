#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(r32i, binding = 0) restrict readonly uniform iimageBuffer indexBuffer;
layout(r32f, binding = 1) restrict readonly uniform imageBuffer positionBuffer;
layout(r32f, binding = 2) restrict readonly uniform imageBuffer normalBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */
	int inIndex = imageLoad(indexBuffer, gl_VertexID).x;

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	inVertexPosition.x = imageLoad(positionBuffer, inIndex * 3 + 0).x; 
	inVertexPosition.y = imageLoad(positionBuffer, inIndex * 3 + 1).x; 
	inVertexPosition.z = imageLoad(positionBuffer, inIndex * 3 + 2).x; 
	
	vec3 inVertexNormal;
	inVertexNormal.x   = imageLoad(normalBuffer, inIndex * 3 + 0).x; 
	inVertexNormal.y   = imageLoad(normalBuffer, inIndex * 3 + 1).x; 
	inVertexNormal.z   = imageLoad(normalBuffer, inIndex * 3 + 2).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

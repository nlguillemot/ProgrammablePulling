#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(r32i, binding = 0) restrict readonly uniform iimageBuffer indexBuffer;
layout(r32f, binding = 1) restrict readonly uniform imageBuffer positionBuffer;
layout(r8, binding = 2) restrict readonly uniform imageBuffer normalBuffer;

out vec3 outVertexPosition;
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
	
	// buffer textures don't support SNORM? :(
	vec3 inVertexNormal;
	inVertexNormal.x   = imageLoad(normalBuffer, inIndex * 3 + 0).x * 2.0 - 1.0; 
	inVertexNormal.y   = imageLoad(normalBuffer, inIndex * 3 + 1).x * 2.0 - 1.0; 
	inVertexNormal.z   = imageLoad(normalBuffer, inIndex * 3 + 2).x * 2.0 - 1.0; 
	
	/* transform vertex and normal */
	outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

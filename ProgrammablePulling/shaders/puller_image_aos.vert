#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(r32i, binding = 0) restrict readonly uniform iimageBuffer indexBuffer;
layout(r32f, binding = 1) restrict readonly uniform imageBuffer attribBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */
	int inIndex = imageLoad(indexBuffer, gl_VertexID).x;

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	vec3 inVertexNormal;
	inVertexPosition.x = imageLoad(attribBuffer, inIndex * 6 + 0).x; 
	inVertexPosition.y = imageLoad(attribBuffer, inIndex * 6 + 1).x; 
	inVertexPosition.z = imageLoad(attribBuffer, inIndex * 6 + 2).x; 
	inVertexNormal.x   = imageLoad(attribBuffer, inIndex * 6 + 3).x; 
	inVertexNormal.y   = imageLoad(attribBuffer, inIndex * 6 + 4).x; 
	inVertexNormal.z   = imageLoad(attribBuffer, inIndex * 6 + 5).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

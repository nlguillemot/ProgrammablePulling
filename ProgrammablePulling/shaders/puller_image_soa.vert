#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;


out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	vec3 inVertexNormal;
	inVertexPosition.x = imageLoad(posXBuffer, inIndex).x; 
	inVertexPosition.y = imageLoad(posYBuffer, inIndex).x; 
	inVertexPosition.z = imageLoad(posZBuffer, inIndex).x; 
	inVertexNormal.x   = imageLoad(normalXBuffer, inIndex).x; 
	inVertexNormal.y   = imageLoad(normalYBuffer, inIndex).x; 
	inVertexNormal.z   = imageLoad(normalZBuffer, inIndex).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

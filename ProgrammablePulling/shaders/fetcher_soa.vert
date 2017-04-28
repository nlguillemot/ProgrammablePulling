#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(binding = 1) uniform samplerBuffer attribBuffer;
layout(binding = 2) uniform samplerBuffer posXBuffer;
layout(binding = 3) uniform samplerBuffer posYBuffer;
layout(binding = 4) uniform samplerBuffer posZBuffer;
layout(binding = 5) uniform samplerBuffer normalXBuffer;
layout(binding = 6) uniform samplerBuffer normalYBuffer;
layout(binding = 7) uniform samplerBuffer normalZBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	vec3 inVertexNormal;
	inVertexPosition.x = texelFetch(posXBuffer, gl_VertexID).x; 
	inVertexPosition.y = texelFetch(posYBuffer, gl_VertexID).x; 
	inVertexPosition.z = texelFetch(posZBuffer, gl_VertexID).x; 
	inVertexNormal.x   = texelFetch(normalXBuffer, gl_VertexID).x; 
	inVertexNormal.y   = texelFetch(normalYBuffer, gl_VertexID).x; 
	inVertexNormal.z   = texelFetch(normalZBuffer, gl_VertexID).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

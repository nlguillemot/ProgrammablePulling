#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(binding = 0) uniform isamplerBuffer indexBuffer;
layout(binding = 1) uniform samplerBuffer posXBuffer;
layout(binding = 2) uniform samplerBuffer posYBuffer;
layout(binding = 3) uniform samplerBuffer posZBuffer;
layout(binding = 4) uniform samplerBuffer normalXBuffer;
layout(binding = 5) uniform samplerBuffer normalYBuffer;
layout(binding = 6) uniform samplerBuffer normalZBuffer;

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch index from texture buffer */
	int inIndex = texelFetch(indexBuffer, gl_VertexID).x;

	/* fetch attributes from texture buffer */
	vec3 inVertexPosition;
	inVertexPosition.x = texelFetch(posXBuffer, inIndex).x; 
	inVertexPosition.y = texelFetch(posYBuffer, inIndex).x; 
	inVertexPosition.z = texelFetch(posZBuffer, inIndex).x; 
	
	// buffer textures don't support SNORM? :(
	vec3 inVertexNormal;
	inVertexNormal.x   = texelFetch(normalXBuffer, inIndex).x * 2.0 - 1.0; 
	inVertexNormal.y   = texelFetch(normalYBuffer, inIndex).x * 2.0 - 1.0; 
	inVertexNormal.z   = texelFetch(normalZBuffer, inIndex).x * 2.0 - 1.0; 
	
	/* transform vertex and normal */
	outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

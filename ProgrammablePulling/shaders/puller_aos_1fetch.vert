layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(binding = 0) uniform isamplerBuffer indexBuffer;
layout(binding = 1) uniform samplerBuffer positionBuffer;
layout(binding = 2) uniform samplerBuffer normalBuffer;

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
	inVertexPosition.xyz = texelFetch(positionBuffer, inIndex).xyz; 
	
	vec3 inVertexNormal;
	inVertexNormal.xyz   = texelFetch(normalBuffer, inIndex).xyz; 
	
	/* transform vertex and normal */
	outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

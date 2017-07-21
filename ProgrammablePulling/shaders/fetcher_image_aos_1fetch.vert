layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(rgba32f, binding = 0) restrict readonly uniform imageBuffer positionBuffer;
layout(rgba32f, binding = 1) restrict readonly uniform imageBuffer normalBuffer;

out vec3 outVertexPosition;
out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch attributes from image buffer */
	vec3 inVertexPosition;
	inVertexPosition.xyz = imageLoad(positionBuffer, gl_VertexID).xyz; 
	
	vec3 inVertexNormal;
	inVertexNormal.xyz   = imageLoad(normalBuffer, gl_VertexID).xyz; 
	
	/* transform vertex and normal */
	outVertexPosition = (Transform.ModelViewMatrix * vec4(inVertexPosition, 1)).xyz;
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

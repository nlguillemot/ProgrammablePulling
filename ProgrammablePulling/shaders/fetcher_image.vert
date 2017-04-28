#version 430 core

layout(std140, binding = 0) uniform transform {
	mat4 ModelViewMatrix;
	mat4 ProjectionMatrix;
	mat4 MVPMatrix;
} Transform;

layout(r32f, binding = 1) restrict readonly uniform imageBuffer attribBuffer;

out vec3 outVertexNormal;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(void) {

	/* fetch attributes from image buffer */
	vec3 inVertexPosition;
	vec3 inVertexNormal;
	inVertexPosition.x = imageLoad(attribBuffer, gl_VertexID * 6 + 0).x; 
	inVertexPosition.y = imageLoad(attribBuffer, gl_VertexID * 6 + 1).x; 
	inVertexPosition.z = imageLoad(attribBuffer, gl_VertexID * 6 + 2).x; 
	inVertexNormal.x   = imageLoad(attribBuffer, gl_VertexID * 6 + 3).x; 
	inVertexNormal.y   = imageLoad(attribBuffer, gl_VertexID * 6 + 4).x; 
	inVertexNormal.z   = imageLoad(attribBuffer, gl_VertexID * 6 + 5).x; 
	
	/* transform vertex and normal */
	outVertexNormal = mat3(Transform.ModelViewMatrix) * inVertexNormal;
	gl_Position = Transform.MVPMatrix * vec4(inVertexPosition, 1);
	
}

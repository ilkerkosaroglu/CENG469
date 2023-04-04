#version 460 core

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;
uniform mat4 controlPoints;
uniform vec4 xyss; // xy offsets, sample, and rectangle size

layout(location=0) in vec3 inVertex;
layout(location=1) in vec3 inNormal;

out vec4 fragWorldPos;
out vec3 fragWorldNor;

out vec4 dbg;

void main(void)
{
	// Determine the indices of the vertex in the 6x6 grid
	int sampleSize = int(xyss.z);
	float sizePerSurface = xyss.w;

	float singleOffset = sizePerSurface / float(sampleSize);

    int x = int((gl_VertexID/6) % sampleSize);
    int y = int((gl_VertexID/6) / sampleSize);

	// Compute the position of the vertex
    vec2 vertexPosition = vec2(float(x) * singleOffset, float(y) * singleOffset);
	int v = gl_VertexID % 6;

	if (v == 1) {
		vertexPosition.x += singleOffset;
	} else if (v == 2) {
		vertexPosition.y += singleOffset;
	} else if (v == 3) {
		vertexPosition.y += singleOffset;
	} else if (v == 4) {
		vertexPosition.x += singleOffset;
	} else if (v == 5) {
		vertexPosition.x += singleOffset;
		vertexPosition.y += singleOffset;
	}

	// shift surfaces
	vertexPosition.x += -0.5 + xyss.x * sizePerSurface;
	vertexPosition.y += -0.5 + xyss.y * sizePerSurface;

	vec3 vertexP = vec3(vertexPosition, 0);

	// Compute the world coordinates of the vertex and its normal.
	// These coordinates will be interpolated during the rasterization
	// stage and the fragment shader will receive the interpolated
	// coordinates.

	fragWorldPos = modelingMatrix * vec4(vertexP, 1);
	fragWorldNor = inverse(transpose(mat3x3(modelingMatrix))) * vec3(0, 0, 1);

	gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(vertexP, 1);
	dbg = projectionMatrix * viewingMatrix * modelingMatrix * vec4(vertexP, 1);

	// int stt=0;
	// // int stt=sampleSize*sampleSize*6-3;
	// if(gl_VertexID >= stt && gl_VertexID < stt+3) {
	// 	if(gl_VertexID%3 == 0) {
	// 		gl_Position = vec4(0, 1, 0, 1);
	// 	}
	// 	if(gl_VertexID%3 == 1) {
	// 		gl_Position = vec4(-1, -1, 0, 1);
	// 	}
	// 	if(gl_VertexID%3 == 2) {
	// 		gl_Position = vec4(1, -1, 0, 1);
	// 	}
	// }
}


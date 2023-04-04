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

void main(void)
{
	// Determine the indices of the vertex in the 6x6 grid
	int s = int(xyss.z);
	float siz = xyss.w;
	int tots = s*s*6;
	float singleOffset = siz / float(s);

    int x = int((gl_VertexID/6) % s);
    int y = int((gl_VertexID/6) / s);

	// Compute the position of the vertex
    vec2 vertexPosition = vec2(float(x) * singleOffset, float(y) * singleOffset);
	int v = gl_VertexID % 6;

	if (v == 1) {
		vertexPosition.y += singleOffset;
	} else if (v == 2) {
		vertexPosition.x += singleOffset;
	} else if (v == 3) {
		vertexPosition.x += singleOffset;
	} else if (v == 4) {
		vertexPosition.y += singleOffset;
	} else if (v == 5) {
		vertexPosition.x += singleOffset;
		vertexPosition.y += singleOffset;
	}

	// shift surfaces
	vertexPosition.x += -0.5 + xyss.x * siz;
	vertexPosition.y += -0.5 + xyss.y * siz;

	// Compute the world coordinates of the vertex and its normal.
	// These coordinates will be interpolated during the rasterization
	// stage and the fragment shader will receive the interpolated
	// coordinates.

	fragWorldPos = modelingMatrix * vec4(vertexPosition, 0, 1);
	fragWorldNor = inverse(transpose(mat3x3(modelingMatrix))) * vec3(0, 0, 1);

	if(gl_VertexID%3 == 0) {
		gl_Position = vec4(0, 1, 0, 1);
	}
	if(gl_VertexID%3 == 1) {
		gl_Position = vec4(-1, -1, 0, 1);
	}
	if(gl_VertexID%3 == 2) {
		gl_Position = vec4(1, -1, 0, 1);
	}

    // gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(inVertex, 1);
}


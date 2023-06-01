#version 460 core

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;
uniform mat4 controlPoints;
uniform vec4 xyss; // xy offsets, sample, and rectangle size
uniform float coordMultiplier;

layout(location=0) in vec3 inVertex;
layout(location=1) in vec3 inNormal;

out vec4 fragWorldPos;
out vec3 fragWorldNor;

// float bin3(int k)
// {	
// 	if(k==0 || k==3) return 1.0;
// 	if(k==1 || k==2) return 3.0;
// 	return 0.0;
// }

float bern(int i, float u)
{
	float ou = 1.0 - u;
	if(i==0){
		// (1-u)^3
		return ou * ou * ou;
	}
	if(i==1){
		// 3 * u * (1-u)^2
		return 3.0 * u * ou * ou;
	}
	if(i==2){
		// 3 * u^2 * (1-u)
		return 3.0 * u * u * ou;
	}
	if(i==3){
		// u^3
		return u * u * u;
	}
	return 0.0;
    // return bin3(i) * max(0.0,pow(u, i)) * max(0.0, pow(1. - u, 3 - i)); 
}
float bernDifferential(int i, float u)
{
	float ou = 1.0 - u;
	if(i==0){
		// (1-u)^3
		return -3.0 * ou * ou;
	}
	if(i==1){
		// 3 * u * (1-u)^2
		float firstTerm = ou * ou;
		float secondTerm = u * (-2. * ou);
		return 3.0 * (firstTerm+secondTerm);
	}
	if(i==2){
		// 3 * u^2 * (1-u)
		float firstTerm = 2. * u * ou;
		float secondTerm = -u*u;
		return 3.0 * (firstTerm+secondTerm);
	}
	if(i==3){
		// u^3
		return 3.0 * u*u;
	}
	return 0.0;
	// float firstTerm = float(i) * pow(u, float(i-1)) * pow(1. - u, float(3 - i));
	// float secondTerm = pow(u, float(i)) * float(3-i) * pow(1. - u, float(2 - i));
    // return bin3(i) * ( firstTerm + secondTerm );
}

out vec3 dbg;

void main(void)
{
	// Determine the indices of the vertex in the 6x6 grid
	int sampleSize = int(xyss.z);
	float sizePerSurface = xyss.w * coordMultiplier;
	float singleOffset = sizePerSurface / float(sampleSize);

    int x = int((gl_VertexID/6) % sampleSize);
    int y = int((gl_VertexID/6) / sampleSize);

	// Compute the position of the vertex
    vec2 vertexPosition = vec2(float(x) * singleOffset, float(y) * singleOffset);
	int v = gl_VertexID % 6;

	float unitSize = 1.0 / float(sampleSize);
	float s = x*unitSize;
	float t = y*unitSize;

	if (v == 1) {
		s += unitSize;
	} else if (v == 2) {
		t += unitSize;
	} else if (v == 3) {
		t += unitSize;
	} else if (v == 4) {
		s += unitSize;
	} else if (v == 5) {
		s += unitSize;
		t += unitSize;
	}

	vec3 finalpos = vec3(0, 0, 0);
	vec3 ds = vec3(0, 0, 0);
	vec3 dt = vec3(0, 0, 0);
	vec3 controlPoint = vec3(0, 0, 0);

	float sizePerCP = sizePerSurface / 3.0;

	int k=0;
	for(;k<16;k++){
		int i = k / 4; //y
		int j = k % 4; //x

		controlPoint = vec3(j*sizePerCP,i*sizePerCP,controlPoints[3-i][j]);

		finalpos += bern(j, s) * bern(i, t) * controlPoint;
		ds += bernDifferential(j, s) * bern(i, t) * controlPoint;
		dt += bern(j, s) * bernDifferential(i, t) * controlPoint;
	}

	vec3 normal = normalize(cross(ds, dt));

	finalpos.x += -coordMultiplier/2.0 + xyss.x * sizePerSurface;
	finalpos.y += -coordMultiplier/2.0 + xyss.y * sizePerSurface;

	fragWorldPos = modelingMatrix * vec4(finalpos, 1);
	fragWorldNor = inverse(transpose(mat3x3(modelingMatrix))) * normal;

	gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(finalpos, 1);

}


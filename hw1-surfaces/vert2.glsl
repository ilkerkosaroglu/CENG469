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

#define PI 3.141592

// Approximation factorial
// float fact(float x)
// {
//    x = max(x, 1.0);
//    return sqrt(2.*PI*x)*pow(x/exp(1.), x)*exp(1./(12.*x)-1./(360.*pow(x,3.)));
// }

int factorial(int n) {
  int result = 1;
  for (int i = 2; i <= n; i++) {
    result *= i;
  }
  return result;	
}



// float binomial(float n, float k)
// {
//     return fact(n) / (fact(k) * fact(n-k));
// }

float binomial(int n, int k)
{
    return float(factorial(n)) / float(factorial(k) * factorial(n-k));
}

// eval of bernstein polynomial, following this formula :
//  bern(m,i) (u) = C(m,i) * u^i * (1-u)^(m-i)
//  - with C(m,i) a binomial coef following this formula :
//    C(n,k) = n!/(k!(n-k)!)
float bern(int i, int m, float u)
{
    return binomial(m, i) * pow(u, i) * pow(1. - u, m - i);
}

out vec3 dbg;

void main(void)
{
	// Determine the indices of the vertex in the 6x6 grid
	int sampleSize = int(xyss.z);
	float sizePerSurface = xyss.w;
	float totalSamples = float(sampleSize * sampleSize);

	float unitSize = 1.0 / float(sampleSize);
	float singleOffset = sizePerSurface / float(sampleSize);

    int x = int((gl_VertexID/6) % sampleSize);
    int y = int((gl_VertexID/6) / sampleSize);

	// Compute the position of the vertex
    vec2 vertexPosition = vec2(float(x) * singleOffset, float(y) * singleOffset);
	int v = gl_VertexID % 6;

	float s = x*unitSize;
	float t = y*unitSize;

	if (v == 1) {
		s += unitSize;
		vertexPosition.x += singleOffset;
	} else if (v == 2) {
		t += unitSize;
		vertexPosition.y += singleOffset;
	} else if (v == 3) {
		t += unitSize;
		vertexPosition.y += singleOffset;
	} else if (v == 4) {
		s += unitSize;
		vertexPosition.x += singleOffset;
	} else if (v == 5) {
		s += unitSize;
		t += unitSize;
		vertexPosition.x += singleOffset;
		vertexPosition.y += singleOffset;
	}

	//  s = vertexPosition.x / sizePerSurface;
	//  t = vertexPosition.y / sizePerSurface;
	// float s = vertexPosition.x / sizePerSurface;
	// float t = vertexPosition.y / sizePerSurface;

	// shift surfaces
	// vertexPosition.x += -0.5 + xyss.x * sizePerSurface;
	// vertexPosition.y += -0.5 + xyss.y * sizePerSurface;

	// vec3 vertexP = vec3(vertexPosition, 0);
	vec3 normal = vec3(0, 0, 1);

	// Compute the world coordinates of the vertex and its normal.
	// 	for idx1 = 1:20
	//     for idx2 = 1:20
	//         val = [0 0 0];
	//         s = idx1 / 20.0;
	//         t = idx2 / 20.0;
	//         for i = 0:3
	//             for j = 0:3
	//                 val = val + bern(i, 3, s) * bern(j, 3, t) * P{i+1, j+1};
	//             end
	//         end
	//         Qx(idx1, idx2) = val(1);
	//         Qy(idx1, idx2) = val(2);
	//         Qz(idx1, idx2) = val(3);
	//     end
	// end

	// float s = float(x) / float(sampleSize);
	// float t = float(y) / float(sampleSize);

	vec3 finalpos = vec3(0, 0, 0);
	vec3 controlPoint = vec3(0, 0, 0);
	// int i = 0; int j = 0;
	// i = 0; j = 0;
	float sizePerCP = sizePerSurface / 3.0;
	int k=0;
	for(;k<16;k++){
		int i = k / 4; //y
		int j = k % 4; //x
		finalpos += bern(j, 3, s) * bern(i, 3, t) * vec3(j*sizePerCP,i*sizePerCP,controlPoints[3-i][j]);
	}

	finalpos.x += -0.5 + xyss.x * sizePerSurface;
	finalpos.y += -0.5 + xyss.y * sizePerSurface;
	// finalpos = vec3(vertexP.xy, finalpos.z);

	fragWorldPos = modelingMatrix * vec4(finalpos, 1);
	fragWorldNor = inverse(transpose(mat3x3(modelingMatrix))) * normal;

	// if(gl_VertexID<120){
	// 	gl_Position = vec4(0,0,0,1);
	// 	return;
	// }
	// if(gl_VertexID<6){
		gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(finalpos, 1);
	// }
	// gl_Position = vsec4(0,0,0,1);
	// dbg = fact(5.5) * vec3(0.002, 0.002, 0.002);
	// dbg = bern(0, 3, s) * vec3(1,1,1);
	dbg = vec3(1,1,1);
	// dbg = vec3(s,s,s);
}


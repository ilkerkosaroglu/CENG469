#version 460 core

uniform vec3 eyePos;

in mat4 inverseViewingMatrix;
in vec4 pos;
out vec4 fragColor;

float stepSize = 0.5;
const int maxSteps = 1000;

vec3 skyColor = vec3(0.2, 0.4, 0.69);
vec3 cloudColor = vec3(1.0, 1.0, 1.0);
float absorptionCoeff = stepSize * 0.20;

vec3 gradients[16] = {
	vec3(1, 1, 0),
	vec3(-1, 1, 0),
	vec3(1, -1, 0),
	vec3(-1, -1, 0),
	vec3(1, 0, 1),
	vec3(-1, 0, 1),
	vec3(1, 0, -1),
	vec3(-1, 0, -1),
	vec3(0, 1, 1),
	vec3(0, -1, 1),
	vec3(0, 1, -1),
	vec3(0, -1, -1),
	vec3(1, 1, 0),
	vec3(-1, 1, 0),
	vec3(0, -1, 1),
	vec3(0, -1, -1)
};

int table[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

float f(float x){
	x = abs(x);
	if(x > 1.0) return 0.0;
	float x2 = x * x;
	float x4 = x2 * x2;
	return -6.0 * x4 * x + 15.0 * x4 - 10.0 * x2 * x + 1.0;
}



vec3 getGradient(ivec3 p){
	int idx;
	idx = table[abs(p.z) % 16];
	idx = table[abs(p.y + idx) % 16];
	idx = table[abs(p.x + idx) % 16];
	return gradients[idx];
}

float noiseOpacity(vec3 p){
	if(p.y < 0.0) return 0.0;
	if(p.y > 10.0) return 0.0;

	p *= 0.05;
	ivec3 ijk = ivec3(p);
	float c = 0.0;
	for(int i=0;i<8;i++){
		ivec3 di = ivec3(i / 4, (i / 2) % 2, i % 2);
		ivec3 corner = ijk + di;
		vec3 g = getGradient(corner);
		vec3 d = vec3(p) - vec3(ijk);
		float weight = f(d.x) * f(d.y) * f(d.z);
		c += weight * dot(g, d);
	}
	return abs(c);
}

void main(void)
{
	vec4 pos4 = inverseViewingMatrix * pos;
	vec3 dir = normalize(pos4.xyz/pos4.w);
	
	vec3 rayPos = eyePos;
	vec3 rayDir = dir;
	vec3 color = skyColor;
	float transparency = 1.0;

	rayPos += rayDir * stepSize * maxSteps;
	for(int i = 0; i < maxSteps; i++){
		float opacity = noiseOpacity(rayPos);
		float absorbed = opacity * absorptionCoeff;

		transparency *= 1.0 - absorbed;

		color = mix(cloudColor, color, absorbed);
		rayPos -= rayDir * stepSize;
	}

	// float op = noiseOpacity(eyePos.xyz);
	// fragColor = vec4(op,op,op, 1);
	fragColor = vec4(color, 1-transparency);
}

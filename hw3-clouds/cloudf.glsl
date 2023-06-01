#version 460 core

uniform vec3 eyePos;

in mat4 inverseViewingMatrix;
in vec4 pos;
out vec4 fragColor;

float stepSize = 0.1;
const int maxSteps = 500;

vec3 skyColor = vec3(0.2, 0.4, 0.69);
vec3 cloudColor = vec3(1.0, 1.0, 1.0);
float absorptionCoeff = stepSize * 1.0;

float noiseOpacity(vec3 p){
	// return fract(sin(dot(p, vec3(12.9898, 78.233, 45.5432))) * 43758.5453);
	return abs(sin(p.x) + sin(p.y) + sin(p.z)) * 0.05;
	// if(length(p) < 10.5){
	// 	return 1.0;
	// }
	// return 0.0;
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
		color = absorbed * cloudColor + (1.0 - absorbed) * color;
		rayPos -= rayDir * stepSize;
	}

	fragColor = vec4(color, 1-transparency);
}

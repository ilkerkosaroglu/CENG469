#version 460 core

uniform vec3 eyePos;

in mat4 inverseViewingMatrix;
in vec3 dir;
out vec4 fragColor;

const float stepSize = 1.0;
const int maxSteps = 300;
const float far = 100;

const vec3 skyColor = vec3(0.2, 0.4, 0.69);
const vec3 cloudColor = vec3(1.0, 1.0, 1.0);
const vec3 denseColor = vec3(0.4,0.4,0.4);

const float cloudSize = 100;
const float cloudGaps = 80;
const float cloudStart = 50.0;
const float cloudEnd = 100.0;

const float absorptionCoeff = stepSize * 0.3;

vec3 gradients[16] = {
	(vec3(1, 1, 0)),
	(vec3(-1, 1, 0)),
	(vec3(1, -1, 0)),
	(vec3(-1, -1, 0)),
	(vec3(1, 0, 1)),
	(vec3(-1, 0, 1)),
	(vec3(1, 0, -1)),
	(vec3(-1, 0, -1)),
	(vec3(0, 1, 1)),
	(vec3(0, -1, 1)),
	(vec3(0, 1, -1)),
	(vec3(0, -1, -1)),
	(vec3(1, 1, 0)),
	(vec3(-1, 1, 0)),
	(vec3(0, -1, 1)),
	(vec3(0, -1, -1))
};


int perm[256]= {151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

vec3 getGradient(ivec3 p){
	int idx;
	ivec3 a = abs(p);
	idx = perm[a.z & 255];
	idx = perm[(a.y+idx)& 255];
	idx = perm[(a.x + idx) & 255];
	return gradients[idx & 15];
}

float noiseOpacity(vec3 p){
	ivec3 ijk = ivec3(floor(p));
	vec3 d = fract(p);
	float c0 = smoothstep(1,0,d.x) * smoothstep(1,0,d.y) * smoothstep(1,0,d.z) * dot(getGradient(ijk), (d));
	float c1 = smoothstep(1,0,1.0 - d.x) * smoothstep(1,0,d.y) * smoothstep(1,0,d.z) * dot(getGradient(ijk + ivec3(1, 0, 0)), (d - vec3(1, 0, 0)));
	float c2 = smoothstep(1,0,d.x) * smoothstep(1,0,1.0 - d.y) * smoothstep(1,0,d.z) * dot(getGradient(ijk + ivec3(0, 1, 0)), (d - vec3(0, 1, 0)));
	float c3 = smoothstep(1,0,d.x) * smoothstep(1,0,d.y) * smoothstep(1,0,1.0 - d.z) * dot(getGradient(ijk + ivec3(0, 0, 1)), (d - vec3(0, 0, 1)));
	float c4 = smoothstep(1,0,1.0 - d.x) * smoothstep(1,0,1.0 - d.y) * smoothstep(1,0,d.z) * dot(getGradient(ijk + ivec3(1, 1, 0)), (d - vec3(1, 1, 0)));
	float c5 = smoothstep(1,0,1.0 - d.x) * smoothstep(1,0,d.y) * smoothstep(1,0,1.0 - d.z) * dot(getGradient(ijk + ivec3(1, 0, 1)), (d - vec3(1, 0, 1)));
	float c6 = smoothstep(1,0,d.x) * smoothstep(1,0,1.0 - d.y) * smoothstep(1,0,1.0 - d.z) * dot(getGradient(ijk + ivec3(0, 1, 1)), (d - vec3(0, 1, 1)));
	float c7 = smoothstep(1,0,1.0 - d.x) * smoothstep(1,0,1.0 - d.y) * smoothstep(1,0,1.0 - d.z) * dot(getGradient(ijk + ivec3(1, 1, 1)), (d - vec3(1, 1, 1)));

	float c = c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7;
	if(c>0)c*=cloudGaps;
	return (c+1.0)/(cloudGaps+1);
}

float pows4[3] = {1.0, pow(4.0, -1.0), pow(4.0, -2.0)};
float pows2[3] = {1.0, pow(2.0, 1.0), pow(2.0, 2.0)};

void main(void)
{
	vec3 rayPos = eyePos;
	const vec3 rayDir = normalize(dir);
	vec3 color = skyColor;
	float transparency = 1.0;
	
	float maxOpacity = 0;
	for(int j = 0; j < 3; j++){
		maxOpacity += pows4[j];
	}

	// rayPos += rayDir * stepSize * maxSteps;
	//hit ray to cloud limit
	if(eyePos.y < cloudStart){
		float t = (cloudStart - eyePos.y)/rayDir.y;
		if(t<0)discard;
		// if(t>far)discard;
		// if(t>15*far)discard;
		if(length((rayDir).xz)>0.999 && t>3*far)discard;
		// if(length((rayDir * t).xz)>far)discard;
		rayPos += rayDir * t;
		// if(length(rayPos- eyePos) > maxSteps)discard;
	}
	if(eyePos.y > cloudEnd){
		float t = (cloudEnd-eyePos.y)/rayDir.y;
		if(t<0)discard;
		// if(t>15*far)discard;
		if(length((rayDir).xz)>0.999 && t>3*far)discard;
		// if(length((rayDir * t).xz)>far)discard;
		rayPos += rayDir * t;
		// if(length(rayPos- eyePos) > maxSteps)discard;
	}
	// calculate only 100 units of ray
	rayPos += rayDir * stepSize * maxSteps;

	// int hitCount = 0;
	for(int i = 0; i < maxSteps; i++){
			if(rayPos.y < cloudStart || rayPos.y > cloudEnd){
				rayPos -= rayDir * stepSize;
				continue;
			}

			float opacity = 0.0;
			for(int j=0;j<3;j++){
				vec3 noisePos = rayPos*pows2[j]/cloudSize;
				opacity += (pows4[j]*noiseOpacity(noisePos))/maxOpacity;
			}

			float absorbed = opacity * absorptionCoeff;
			transparency *= 1.0 - absorbed;
			
			color = mix(color, mix(cloudColor, denseColor, opacity), absorbed);
		
		rayPos -= rayDir * stepSize;
	}

	// float op = noiseOpacity(eyePos.xyz);
	// fragColor = vec4(op,op,op, 1);
	// fragColor = vec4(color, 1);
	fragColor = vec4(color, 1-transparency);
}

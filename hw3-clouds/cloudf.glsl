#version 460 core

uniform vec3 eyePos;

in mat4 inverseViewingMatrix;
in vec4 pos;
out vec4 fragColor;

float stepSize = 3.0;
const int maxSteps = 300;

vec3 skyColor = vec3(0.2, 0.4, 0.69);
vec3 cloudColor = vec3(1.0, 1.0, 1.0);
vec3 denseColor = vec3(0.5,0.5,0.5);

float cloudSize = 100;
float cloudGaps = 50;
float cloudStart = 0.0;
float cloudEnd = 100.0;

float absorptionCoeff = stepSize * 0.3;

vec3 sunPos = vec3(0.0, 100.0, 0.0);

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


float f(float x){
	return 1 - x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
	// float x2 = x * x;
	// float x4 = x2 * x2;
	// return -6.0 * x4 * x + 15.0 * x4 - 10.0 * x2 * x + 1.0;
}



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
	float c = 0.0;
	vec3 d = fract(p);
	c += f(d.x) * f(d.y) * f(d.z) * dot(getGradient(ijk), (d));
	c += f(1.0 - d.x) * f(d.y) * f(d.z) * dot(getGradient(ijk + ivec3(1, 0, 0)), (d - vec3(1, 0, 0)));
	c += f(d.x) * f(1.0 - d.y) * f(d.z) * dot(getGradient(ijk + ivec3(0, 1, 0)), (d - vec3(0, 1, 0)));
	c += f(d.x) * f(d.y) * f(1.0 - d.z) * dot(getGradient(ijk + ivec3(0, 0, 1)), (d - vec3(0, 0, 1)));
	c += f(1.0 - d.x) * f(1.0 - d.y) * f(d.z) * dot(getGradient(ijk + ivec3(1, 1, 0)), (d - vec3(1, 1, 0)));
	c += f(1.0 - d.x) * f(d.y) * f(1.0 - d.z) * dot(getGradient(ijk + ivec3(1, 0, 1)), (d - vec3(1, 0, 1)));
	c += f(d.x) * f(1.0 - d.y) * f(1.0 - d.z) * dot(getGradient(ijk + ivec3(0, 1, 1)), (d - vec3(0, 1, 1)));
	c += f(1.0 - d.x) * f(1.0 - d.y) * f(1.0 - d.z) * dot(getGradient(ijk + ivec3(1, 1, 1)), (d - vec3(1, 1, 1)));

	if(c>0)c*=cloudGaps;
	return (c+1.0)/(cloudGaps+1);
	// return (c+1.0)/2.0;
	// return abs(c);
	// for(int i=0;i<8;i++){
	// 	ivec3 di = ivec3(i / 4, (i / 2) % 2, i % 2);
	// 	ivec3 corner = ijk + di;
	// 	vec3 g = getGradient(corner);
	// 	vec3 d = vec3(p) - vec3(corner);
	// 	float weight = f(abs(d.x)) * f(abs(d.y)) * f(abs(d.z));
	// 	c += weight * dot(g, d);
	// }
	// return (c+1.0)/2.0;
	// return abs(c);
}

float rand(vec3 co){
    // return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
	return fract(sin(dot(co, vec3(12.9898, 78.233, 45.5432))) * 43758.5453);
}

float valueNoise(vec3 p) 
{
    vec3 u = floor(p);
    vec3 v = fract(p);
    vec3 s = smoothstep(0.0, 1.0, v);
    
    float a = rand(u);
    float b = rand(u + vec3(1.0, 0.0, 0.0));
    float c = rand(u + vec3(0.0, 1.0, 0.0));
    float d = rand(u + vec3(1.0, 1.0, 0.0));
    float e = rand(u + vec3(0.0, 0.0, 1.0));
    float f = rand(u + vec3(1.0, 0.0, 1.0));
    float g = rand(u + vec3(0.0, 1.0, 1.0));
    float h = rand(u + vec3(1.0, 1.0, 1.0));
    
	float res = mix(mix(mix(a, b, s.x), mix(c, d, s.x), s.y),
               mix(mix(e, f, s.x), mix(g, h, s.x), s.y),
               s.z);
    float unnor = (res*2.0)-1.0;
	if(unnor>0)unnor*=cloudGaps;
	return (unnor+1)/(cloudGaps+1);
}


void main(void)
{
	vec4 pos4 = inverseViewingMatrix * pos;
	vec3 dir = normalize(pos4.xyz/pos4.w);
	
	vec3 rayPos = eyePos;
	vec3 rayDir = dir;
	vec3 color = skyColor;
	float transparency = 1.0;
	
	float maxOpacity = 0;
	for(int j = 0; j < 3; j++){
		maxOpacity += pow(4.0, - float(j));
	}

	rayPos += rayDir * stepSize * maxSteps;
	bool hitOnce = false;
	for(int i = 0; i < maxSteps; i++){
			if(rayPos.y < cloudStart || rayPos.y > cloudEnd){
				if(hitOnce){
					break;
				}
				rayPos -= rayDir * stepSize;
				continue;
			}
			hitOnce = true;
			vec3 sunDist = rayPos - sunPos.xyz;
			vec3 sunDir = normalize(sunDist);
			vec3 sunStepSize = sunDist / 100.0;
			vec3 curPos = sunPos;

			float opacity = 0.0;
			// float mult = 1.0;
			for(int j=0;j<3;j++){
				// for(int j=0;j<100;j++){
				// 	vec3 curpos = crPos + sunDir * sunStepSize * float(j);
				// 	float opacity = noiseOpacity(rayPos*pow(2.0, float(j)));
				// }
				vec3 noisePos = rayPos*pow(2.0, float(j))/cloudSize;
				opacity += (pow(4.0, -float(j))*valueNoise(noisePos))/maxOpacity;
				// opacity += (pow(4.0, -float(j))*noiseOpacity(noisePos))/maxOpacity;
				// if(rayPos.y < cloudStart-3 || rayPos.y > cloudEnd+3){
				// 	continue;
				// }
				// if(rayPos.y < cloudStart || rayPos.y > cloudEnd){
				// 	mult = 0.01;
				// }
				// opacity += (mult*pow(4.0, -float(j))*noiseOpacity(rayPos*pow(2.0, float(j))))/maxOpacity;
			}
			float absorbed = opacity * absorptionCoeff;
			transparency *= 1.0 - absorbed;
			
			// if(rayPos.y+3.0 > cloudEnd){
			// 	denseColor = cloudColor;
			// }
			color = mix(color, cloudColor, absorbed);
			// color = mix(color, mix(cloudColor, denseColor, opacity*3), absorbed);
		
		rayPos -= rayDir * stepSize;
	}

	// float op = noiseOpacity(eyePos.xyz);
	// fragColor = vec4(op,op,op, 1);
	// fragColor = vec4(color, 1);
	fragColor = vec4(color, 1-transparency);
}

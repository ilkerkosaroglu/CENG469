#version 460 core

vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient
vec3 kd = vec3(0.8, 0.8, 0.8);   // diffuse reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient

struct Light
{
	vec3 position;
	vec3 intensity;
};

const int MAX_LIGHTS = 5;

uniform Light light[MAX_LIGHTS];

uniform vec3 eyePos;

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

in vec3 dbg;
void main(void)
{
	int i=0;
	vec3 col = vec3(0,0,0);
	for(; i < MAX_LIGHTS; i++){
		// Compute lighting. We assume lightPos and eyePos are in world
		// coordinates. fragWorldPos and fragWorldNor are the interpolated
		// coordinates by the rasterizer.
		vec3 lightPos = light[i].position;
		vec3 I = light[i].intensity;

		vec3 L = normalize(lightPos - vec3(fragWorldPos));
		vec3 V = normalize(eyePos - vec3(fragWorldPos));
		vec3 H = normalize(L + V);
		vec3 N = normalize(fragWorldNor);

		float NdotL = dot(N, L); // for diffuse component
		float NdotH = dot(N, H); // for specular component

		float len = length(lightPos - vec3(fragWorldPos));
		I *= 1.0 / (len * len);

		vec3 diffuseColor = I * kd * max(0, NdotL);
		vec3 specularColor = I * ks * pow(max(0.00001, NdotH), 500.0);
		
		col += diffuseColor + specularColor;
	}
	vec3 ambientColor = Iamb * ka;
	fragColor = vec4(col+ambientColor, 1);

}

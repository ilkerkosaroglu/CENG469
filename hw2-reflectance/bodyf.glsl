#version 460 core

uniform samplerCube skybox;
uniform sampler2D matcap;
uniform vec3 eyePos;

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

void main(void)
{
	vec3 V = normalize(eyePos - vec3(fragWorldPos));
	vec3 N = normalize(fragWorldNor);

	vec3 reflectDir = reflect(-V, N);

	float m = 2.8284271247461903 * sqrt( reflectDir.z+1.0 );
	vec2 mcuv = reflectDir.xy / m + 0.5;
	vec4 mccolor = vec4(texture(matcap, mcuv).xyz,1);

	// fragColor = vec4(reflectDir, 1);
	// fragColor = vec4(texture(skybox, reflectDir).xyz, 1);
	reflectDir.y *= -1;
	reflectDir.z *= -1;
	fragColor = mix(mccolor, texture(skybox, reflectDir), 0.3);
}

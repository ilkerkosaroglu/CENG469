#version 460 core

uniform samplerCube skybox;
uniform vec3 eyePos;

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

void main(void)
{
	vec3 V = normalize(eyePos - vec3(fragWorldPos));
	vec3 N = normalize(fragWorldNor);


	vec4 mccolor = vec4(0,1,1,1);

	vec3 reflectDir = reflect(-V, N);
	reflectDir.y *= -1;
	reflectDir.z *= -1;
	fragColor = mix(mccolor, texture(skybox, reflectDir), 0.9);
}

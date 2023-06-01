#version 460 core

in mat4 inverseViewingMatrix;
in vec4 pos;
out vec4 fragColor;

uniform samplerCube skybox;

void main(void)
{
	vec4 pos4 = inverseViewingMatrix * pos;
	vec3 dir = normalize(pos4.xyz/pos4.w);

	// since after inverseViewingMatrix, the coordinate is left-handed
	dir.z = -dir.z;
	dir.y = -dir.y;

	fragColor = vec4(texture(skybox, dir).xyz, 1.0);

}

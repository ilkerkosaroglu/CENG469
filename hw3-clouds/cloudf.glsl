#version 460 core

in mat4 inverseViewingMatrix;
in vec4 pos;
out vec4 fragColor;

void main(void)
{
	vec4 pos4 = inverseViewingMatrix * pos;
	vec3 dir = normalize(pos4.xyz/pos4.w);

	fragColor = vec4(dir.xyz, 0.5);

}

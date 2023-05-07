#version 460 core

in mat4 inverseViewingMatrix;
out vec4 fragColor;

uniform samplerCube skybo;

void main(void)
{
	// Set the color of this fragment to the interpolated color
	// value computed by the rasterizer.

	// fragColor = vec4(texture(skybo, gl_FragCoord.xyz/500.0).xyz, 1.0);
	// fragColor = vec4(texture(skybo, gl_FragCoord.xy/500.0).xyz, 1.0);
	vec4 pos = inverseViewingMatrix * gl_FragCoord;
	fragColor = vec4(texture(skybo, pos.xyz/pos.w).xyz, 1.0);
	// fragColor = texture(tex0, gl_FragCoord.xy/500.0);
}

#version 460 core

in vec4 color;
out vec4 fragColor;

uniform sampler2D tex0;

void main(void)
{
	// Set the color of this fragment to the interpolated color
	// value computed by the rasterizer.

	fragColor = vec4(texture(tex0, gl_FragCoord.xy/500.0).xyz, 1.0);
	// fragColor = texture(tex0, gl_FragCoord.xy/500.0);
}

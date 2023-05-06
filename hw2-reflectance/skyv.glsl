#version 460 core

uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;
uniform vec3 eyePos;

layout(location=0) in vec3 inVertex;

void main(void)
{

    gl_Position = vec4(inVertex.xyz, 1.0);
    // gl_Position = projectionMatrix * viewingMatrix * vec4(inVertex, 1);
}


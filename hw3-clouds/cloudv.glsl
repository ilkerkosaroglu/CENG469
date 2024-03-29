#version 460 core

uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;

layout(location=0) in vec3 inVertex;

out mat4 inverseViewingMatrix;
out vec3 dir;
void main(void)
{

    gl_Position = vec4(inVertex.xy, 1.0, 1.0);
    
    mat4 vM = viewingMatrix;
    vM[3][0] = 0;
    vM[3][1] = 0;
    vM[3][2] = 0;
    // inverseViewingMatrix = inverse(vM);
    // inverseViewingMatrix = inverse(vM);
    inverseViewingMatrix = inverse(projectionMatrix * vM);
    dir = (inverseViewingMatrix * gl_Position).xyz;
    // gl_Position = projectionMatrix * viewingMatrix * vec4(inVertex, 1);
}


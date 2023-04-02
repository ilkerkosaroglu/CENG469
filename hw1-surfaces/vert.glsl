#version 460 core

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them 
// here for simplicity.

vec3 I = vec3(1, 1, 1);          // point light intensity
vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 kd = vec3(1, 0.2, 0.2);     // diffuse reflectance coefficient
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient
vec3 lightPos = vec3(5, 5, 5);   // light position in world coordinates

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;
uniform vec3 eyePos;

layout(location=0) in vec3 inVertex;
layout(location=1) in vec3 inNormal;

out vec4 color;

void main(void)
{
	// First, convert to world coordinates. This is where
	// lighting computations must be performed. inVertex
	// is NOT in homogeneous coordinates. inNormal has three
	// components. For computing the normal transformation
	// matrix we use the upper 3x3 part of the modeling
	// matrix.
	 
	vec4 pWorld = modelingMatrix * vec4(inVertex, 1);
	vec3 nWorld = inverse(transpose(mat3x3(modelingMatrix))) * inNormal;

	// Compute lighting. We assume lightPos and eyePos are in world
	// coordinates.

	vec3 L = normalize(lightPos - vec3(pWorld));
	vec3 V = normalize(eyePos - vec3(pWorld));
	vec3 H = normalize(L + V);
	vec3 N = normalize(nWorld);

	float NdotL = dot(N, L); // for diffuse component
	float NdotH = dot(N, H); // for specular component

	vec3 diffuseColor = I * kd * max(0, NdotL);
	vec3 specularColor = I * ks * pow(max(0, NdotH), 100);
	vec3 ambientColor = Iamb * ka;

	// We update the front color of the vertex. This value will be sent
	// to the fragment shader after it is interpolated at every fragment.
	// Front color specifies the color of a vertex for a front facing
	// primitive.

	color = vec4(diffuseColor + specularColor + ambientColor, 1);

	// Transform the vertex with the product of the projection, viewing, and
	// modeling matrices.

    gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(inVertex, 1);
}


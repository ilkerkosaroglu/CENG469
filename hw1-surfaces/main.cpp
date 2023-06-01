#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/quaternion.hpp>
#define dbg(x) cout << #x << ": " << x << endl;
#define pv(x) cout<<#x<<": ";for(auto k:x){ cout<<k<<" "; }cout<<endl;

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

const int MAX_SAMPLES = 80;
const int MIN_SAMPLES = 2;

class Surface
{
public:
	glm::vec4 xyss; // starting point of the surface
	glm::mat4 cp;	// control point heights
};

vector<Surface> gSurfaces;
int sampleCount = 10;

float curRotation = -30;
float coordMultiplier = 1.0f;

GLuint gProgram;
int gWidth, gHeight;

glm::vec3 gLightPos[5];
glm::vec3 gLightI[5];

GLint modelingMatrixLoc;
GLint viewingMatrixLoc;
GLint projectionMatrixLoc;
GLint eyePosLoc;
GLint controlPointsLoc;
GLint xyssLoc;
GLint coordMultiplierLoc;
GLint lightPosLoc[5];
GLint lightILoc[5];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;
glm::vec3 eyePos(0, 0, 2);

struct Vertex
{
	Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
	GLfloat x, y, z;
};

struct Texture
{
	Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
	GLfloat u, v;
};

struct Normal
{
	Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
	GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
	GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLuint gVertexAttribBuffer, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;


// use this function instead of parseObj
void createSurfaces(){
	GLfloat x = 0;
	for (int i = 0; i < MAX_SAMPLES * MAX_SAMPLES*6; i++)
	gVertices.push_back(Vertex(x, x, x));
	for (int i = 0; i < MAX_SAMPLES * MAX_SAMPLES; i++)
	{
		int v[3] = {i*6, i*6+1, i*6+2};
		gFaces.push_back(Face(v,v,v));
		v[0] = i*6+3;
		v[1] = i*6+4;
		v[2] = i*6+5;
		gFaces.push_back(Face(v,v,v));
	}
}

void initVBOForSurfaces()
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	assert(vao > 0);
	glBindVertexArray(vao);
	// cout << "vao = " << vao << endl;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

	glGenBuffers(1, &gVertexAttribBuffer);
	glGenBuffers(1, &gIndexBuffer);

	assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	gVertexDataSizeInBytes = gVertices.size() * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);
	GLfloat *vertexData = new GLfloat[gVertices.size() * 3];
	GLuint *indexData = new GLuint[gFaces.size() * 3];

	float minX = 1e6, maxX = -1e6;
	float minY = 1e6, maxY = -1e6;
	float minZ = 1e6, maxZ = -1e6;

	for (int i = 0; i < gVertices.size(); ++i)
	{
		vertexData[3 * i] = gVertices[i].x;
		vertexData[3 * i + 1] = gVertices[i].y;
		vertexData[3 * i + 2] = gVertices[i].z;
	}

	for (int i = 0; i < gFaces.size(); ++i)
	{
		indexData[3 * i] = gFaces[i].vIndex[0];
		indexData[3 * i + 1] = gFaces[i].vIndex[1];
		indexData[3 * i + 2] = gFaces[i].vIndex[2];
	}

	// glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, 0, GL_STATIC_DRAW);
	// glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, vertexData, GL_STATIC_DRAW);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

	// done copying; can free now
	delete[] vertexData;
	delete[] indexData;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

bool ReadDataFromFile(
	const string& fileName, ///< [in]  Name of the shader file
	string& data)     ///< [out] The contents of the file
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			data += curLine;
			if (!myfile.eof())
			{
				data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}

bool ReadDataFromFile(
	const string &fileName, ///< [in]  Name of the shader file
	vector<string> &data)			///< [out] The contents of the file
{
	fstream myfile;

	// Open the input
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream ss(curLine);
			string token;
			while(ss >> token){
				data.push_back(token);
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}


void setSampleSize(int newSampleSize)
{
	// loop each surface
	for(auto &k: gSurfaces){
		k.xyss.z = newSampleSize * 1.0f;
	}
}

void ParseObj(const string &fileName)
{
	vector<string> data;
	ReadDataFromFile(fileName, data);

	// for(int i=0; i<data.size();i++){
	// 	cout<<data[i]<<endl;
	// }
	int numLights = stoi(data[0]);
	int height = stoi(data[numLights*6+1]);
	int width = stoi(data[numLights*6+2]);
	int lightsStart = 1;
	int start = numLights*6+3;

	// parse lights
	for(int i=0; i<numLights; i++){
		int index = lightsStart + i * 6;
		gLightPos[i] = glm::vec3(stof(data[index]), stof(data[index + 1]), stof(data[index + 2]));
		gLightI[i] = glm::vec3(stof(data[index + 3]), stof(data[index + 4]), stof(data[index + 5]));
	}

	for(int i=numLights; i<5; i++){
		gLightPos[i] = glm::vec3(0.0f, 0.0f, 0.0f);
		gLightI[i] = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	// parse surfaces
	gSurfaces.resize((height*width)/16);
	float sizePerSurface = 4.0f / max(height, width);
	// loop each surface
	for(int y=0; y<height/4;y++){
		for(int x=0;x<width/4;x++){
			int surfaceIndex = y*(width/4) + x;
			gSurfaces[surfaceIndex].xyss = glm::vec4(x * 1.0f, ((height/4)-y-1) * 1.0f, sampleCount*1.0f, sizePerSurface);
			// loop surface data
			for(int i=0; i<4; i++){
				for(int j=0; j<4; j++){
					int index = start + (y*4+i)*width + x*4 + j;
					gSurfaces[surfaceIndex].cp[i][j] = stof(data[index]);
				}
			}
		}
	}
}

GLuint createVS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &shader, &length);
	glCompileShader(vs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(vs, 1024, &length, output);
	printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &shader, &length);
	glCompileShader(fs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(fs, 1024, &length, output);
	printf("FS compile log: %s\n", output);

	return fs;
}

void initShaders()
{
	// Create the programs

	gProgram = glCreateProgram();

	// Create the shaders for both programs

	GLuint vs = createVS("vert2.glsl");
	GLuint fs = createFS("frag2.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram, vs);
	glAttachShader(gProgram, fs);

	// Link the programs

	glLinkProgram(gProgram);
	GLint status;
	glGetProgramiv(gProgram, GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	// Get the locations of the uniform variables from both programs
	modelingMatrixLoc = glGetUniformLocation(gProgram, "modelingMatrix");
	viewingMatrixLoc = glGetUniformLocation(gProgram, "viewingMatrix");
	projectionMatrixLoc = glGetUniformLocation(gProgram, "projectionMatrix");
	eyePosLoc = glGetUniformLocation(gProgram, "eyePos");
	controlPointsLoc = glGetUniformLocation(gProgram, "controlPoints");
	xyssLoc = glGetUniformLocation(gProgram, "xyss");
	coordMultiplierLoc = glGetUniformLocation(gProgram, "coordMultiplier");

	for (int i = 0; i < 4; ++i){
		string sp = "light[" + to_string(i) + "].position";
		string si = "light[" + to_string(i) + "].intensity";
		lightPosLoc[i] = glGetUniformLocation(gProgram, sp.c_str());
		lightILoc[i] = glGetUniformLocation(gProgram, si.c_str());
	}
}

int polygonMode = GL_FILL;
void init()
{
	createSurfaces();

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, polygonMode);

	initShaders();
	initVBOForSurfaces();
}

void setLights()
{
	for (int i = 0; i < 4; ++i){
		glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(gLightPos[i]));
		glUniform3fv(lightILoc[i], 1, glm::value_ptr(gLightI[i]));
	}
}

void setSurfaceUniforms(int surfaceIndex){
	Surface &surface = gSurfaces[surfaceIndex];

	glUniformMatrix4fv(controlPointsLoc, 1, GL_FALSE, glm::value_ptr(surface.cp));
	glUniform4fv(xyssLoc, 1, glm::value_ptr(surface.xyss));
}

void drawSurface()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, sampleCount*sampleCount*6, GL_UNSIGNED_INT, 0);
}

void display()
{
	glClearColor(0, 0, 0, 1);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static float angle = 0;

	float angleRad = (float)(angle / 180.0) * M_PI;

	// Compute the modeling matrix
	glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(-0.1f, -0.2f, -7.0f));
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), (curRotation / 180.) * M_PI, glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 matRz = glm::rotate<float>(glm::mat4(1.0), angleRad, glm::vec3(0.0, 0.0, 1.0));
	modelingMatrix = matRx;
	// modelingMatrix = glm::mat4(1.0);
	// Set the active program and the values of its uniform variables
	glUseProgram(gProgram);
	viewingMatrix = glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc, 1, glm::value_ptr(eyePos));
	glUniform1f(coordMultiplierLoc, coordMultiplier);
	setLights();

	// Draw surfaces
	for (int i = 0; i < gSurfaces.size(); i++){
		setSurfaceUniforms(i);
		drawSurface();
	}

	angle += 0.5;
}

void reshape(GLFWwindow* window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	//glOrtho(-10, 10, -10, 10, -10, 10);
	//gluPerspective(45, 1, 1, 100);

	// Use perspective projection

	float fovyRad = (float)(45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, w/(float) h, 1.0f, 100.0f);

	// Assume default camera position and orientation (camera is at
	// (0, 0, 0) with looking at -z direction and its up vector pointing
	// at +y direction)

	viewingMatrix = glm::mat4(1);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
}

const bool ALLOW_REPEAT = true;
float getActionMultiplier()
{
	if(ALLOW_REPEAT){
		return 0.2;
	}
	return 1;
}
bool getActionState(int action)
{
	return action == GLFW_PRESS || (action == GLFW_REPEAT && ALLOW_REPEAT);
}
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_Q && getActionState(action))
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_R && getActionState(action))
	{
		curRotation += 10 * getActionMultiplier();
	}
	else if (key == GLFW_KEY_F && getActionState(action))
	{
		curRotation -= 10 * getActionMultiplier();
	}
	else if (key == GLFW_KEY_W && getActionState(action))
	{
		sampleCount += 2;
		sampleCount = min(sampleCount, 80);
		setSampleSize(sampleCount);
	}
	else if (key == GLFW_KEY_S && getActionState(action))
	{
		sampleCount -= 2;
		sampleCount = max(sampleCount, 2);
		setSampleSize(sampleCount);
	}
	else if (key == GLFW_KEY_E && getActionState(action))
	{
		coordMultiplier += 0.1 * getActionMultiplier();
	}
	else if (key == GLFW_KEY_D && getActionState(action))
	{
		coordMultiplier -= 0.1 * getActionMultiplier();
		coordMultiplier = max(coordMultiplier, 0.1f);
	}
	else if (key == GLFW_KEY_V && getActionState(action))
	{
		if(polygonMode == GL_FILL)
			polygonMode = GL_LINE;
		else
			polygonMode = GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
	}
}

void mainLoop(GLFWwindow* window)
{
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
	cout << "GL_DEBUG_TYPE_ERROR:" << (GL_DEBUG_TYPE_ERROR == type) << std::endl;
	cout << "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:" << (GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR == type) << std::endl;
	cout << "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:" << (GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR == type) << std::endl;
	cout << "GL_DEBUG_TYPE_PORTABILITY:" << (GL_DEBUG_TYPE_PORTABILITY == type) << std::endl;
	cout << "GL_DEBUG_TYPE_PERFORMANCE:" << (GL_DEBUG_TYPE_PERFORMANCE == type) << std::endl;
	cout << "GL_DEBUG_TYPE_MARKER:" << (GL_DEBUG_TYPE_MARKER == type) << std::endl;
	cout << "GL_DEBUG_TYPE_PUSH_GROUP:" << (GL_DEBUG_TYPE_PUSH_GROUP == type) << std::endl;
	cout << "GL_DEBUG_TYPE_POP_GROUP:" << (GL_DEBUG_TYPE_POP_GROUP == type) << std::endl;
	cout << "GL_DEBUG_TYPE_OTHER:" << (GL_DEBUG_TYPE_OTHER == type) << std::endl;
	cout << "GL_DONT_CARE:" << (GL_DONT_CARE == type) << std::endl;
}
int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
	GLFWwindow* window;
	if (!glfwInit())
	{
		exit(-1);
	}

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	int width = 800, height = 600;
	window = glfwCreateWindow(width, height, "CENG 469 - HW1 - 2022-2023", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	// char rendererInfo[512] = { 0 };
	// strcpy(rendererInfo, (const char*)glGetString(GL_RENDERER));
	// strcat(rendererInfo, " - ");
	// strcat(rendererInfo, (const char*)glGetString(GL_VERSION));
	// glfwSetWindowTitle(window, rendererInfo);

	if(argc > 1)
		ParseObj(argv[1]);
	else {
		std::cout << "Usage: " << argv[0] << " <input file>" << std::endl;
		exit(-1);
	}

	glDebugMessageCallback(debugCallback, NULL);

	init();

	glfwSetKeyCallback(window, keyboard);
	glfwSetWindowSizeCallback(window, reshape);

	reshape(window, width, height); // need to call this once ourselves
	mainLoop(window); // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

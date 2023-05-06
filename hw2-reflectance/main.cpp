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
// #define dbg(x) ();
#define pv(x) cout<<#x<<": ";for(auto k:x){ cout<<k<<" "; }cout<<endl;

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[2];
int gWidth, gHeight;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;

float eyeRotX = 0;
float eyeRotY = 0;
glm::vec3 eyePos(0, 0, 0);
glm::vec3 orbitCenter = glm::vec3(0.f, 0.f, -7.0f);
glm::vec3 objCenter = glm::vec3(-0.1f, -0.2f, -7.0f);

int activeProgramIndex = 0;

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

class Uniform{
	public:
	string name;
	GLint location;
	string type;
	float value;
};

class Geometry{
	public:
	vector<Vertex> vertices;
	vector<Texture> textures;
	vector<Normal> normals;
	vector<Face> faces;
	void initVBO();
	GLuint vertexAttribBuffer, indexBuffer;
	int vertexDataSizeInBytes, normalDataSizeInBytes;

	void drawModel();
};

// hold textures?
class Material{
	public:
	string name; 
};

class RenderObject{
	public:
	GLint program;
	vector<Uniform> uniforms;
	Geometry geometry;
	string name;

};

vector<RenderObject> rObjects;

bool ParseObj(const string& fileName, const string& name)
{
	fstream myfile;

	rObjects.emplace_back();
	auto &obj = rObjects.back();
	obj.program = gProgram[0];
	obj.name = name;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream str(curLine);
			GLfloat c1, c2, c3;
			GLuint index[9];
			string tmp;

			if (curLine.length() >= 2)
			{
				if (curLine[0] == 'v')
				{
					if (curLine[1] == 't') // texture
					{
						str >> tmp; // consume "vt"
						str >> c1 >> c2;
						obj.geometry.textures.push_back(Texture(c1, c2));
					}
					else if (curLine[1] == 'n') // normal
					{
						str >> tmp; // consume "vn"
						str >> c1 >> c2 >> c3;
						obj.geometry.normals.push_back(Normal(c1, c2, c3));
					}
					else // vertex
					{
						str >> tmp; // consume "v"
						str >> c1 >> c2 >> c3;
						obj.geometry.vertices.push_back(Vertex(c1, c2, c3));
					}
				}
				else if (curLine[0] == 'f') // face
				{
					str >> tmp; // consume "f"
					char c;
					int vIndex[3], nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						vIndex[1] == nIndex[1] &&
						vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

					obj.geometry.faces.push_back(Face(vIndex, tIndex, nIndex));
				}
				else
				{
					cout << "Ignoring unidentified line in obj file: " << curLine << endl;
				}
			}

			//data += curLine;
			if (!myfile.eof())
			{
				//data += "\n";
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

	gProgram[0] = glCreateProgram();
	gProgram[1] = glCreateProgram();

	// Create the shaders for both programs

	GLuint vs1 = createVS("vert.glsl");
	GLuint fs1 = createFS("frag.glsl");

	GLuint vs2 = createVS("vert2.glsl");
	GLuint fs2 = createFS("frag2.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram[0], vs1);
	glAttachShader(gProgram[0], fs1);

	glAttachShader(gProgram[1], vs2);
	glAttachShader(gProgram[1], fs2);

	// Link the programs

	glLinkProgram(gProgram[0]);
	GLint status;
	glGetProgramiv(gProgram[0], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[1]);
	glGetProgramiv(gProgram[1], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	// Get the locations of the uniform variables from both programs

	for (int i = 0; i < 2; ++i)
	{
		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
	}
}

void Geometry::initVBO()
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	assert(vao > 0);
	glBindVertexArray(vao);
	cout << "vao = " << vao << endl;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

	glGenBuffers(1, &this->vertexAttribBuffer);
	glGenBuffers(1, &this->indexBuffer);

	assert(this->vertexAttribBuffer > 0 && this->indexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, this->vertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);

	const int vSize = this->vertices.size();
	dbg(vSize);
	const int nSize = this->normals.size();
	dbg(nSize);
	const int fSize = this->faces.size();
	dbg(fSize);

	this->vertexDataSizeInBytes = vSize * 3 * sizeof(GLfloat);
	this->normalDataSizeInBytes = nSize * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = fSize * 3 * sizeof(GLuint);
	GLfloat *vertexData = new GLfloat[vSize * 3];
	GLfloat *normalData = new GLfloat[nSize * 3];
	GLuint *indexData = new GLuint[fSize * 3];

	float minX = 1e6, maxX = -1e6;
	float minY = 1e6, maxY = -1e6;
	float minZ = 1e6, maxZ = -1e6;

	for (int i = 0; i < vSize; ++i)
	{
		vertexData[3 * i] = this->vertices[i].x;
		vertexData[3 * i + 1] = this->vertices[i].y;
		vertexData[3 * i + 2] = this->vertices[i].z;

		minX = std::min(minX, this->vertices[i].x);
		maxX = std::max(maxX, this->vertices[i].x);
		minY = std::min(minY, this->vertices[i].y);
		maxY = std::max(maxY, this->vertices[i].y);
		minZ = std::min(minZ, this->vertices[i].z);
		maxZ = std::max(maxZ, this->vertices[i].z);
	}

	std::cout << "minX = " << minX << std::endl;
	std::cout << "maxX = " << maxX << std::endl;
	std::cout << "minY = " << minY << std::endl;
	std::cout << "maxY = " << maxY << std::endl;
	std::cout << "minZ = " << minZ << std::endl;
	std::cout << "maxZ = " << maxZ << std::endl;

	for (int i = 0; i < nSize; ++i)
	{
		normalData[3 * i] = this->normals[i].x;
		normalData[3 * i + 1] = this->normals[i].y;
		normalData[3 * i + 2] = this->normals[i].z;
	}

	for (int i = 0; i < fSize; ++i)
	{
		indexData[3 * i] = this->faces[i].vIndex[0];
		indexData[3 * i + 1] = this->faces[i].vIndex[1];
		indexData[3 * i + 2] = this->faces[i].vIndex[2];
	}


	glBufferData(GL_ARRAY_BUFFER, this->vertexDataSizeInBytes + this->normalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, this->vertexDataSizeInBytes, vertexData);
	glBufferSubData(GL_ARRAY_BUFFER, this->vertexDataSizeInBytes, this->normalDataSizeInBytes, normalData);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

	// done copying; can free now
	delete[] vertexData;
	delete[] normalData;
	delete[] indexData;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(this->vertexDataSizeInBytes));
}

void init()
{
	ParseObj("hw2_support_files/obj/armadillo.obj", "armadillo");
	//ParseObj("bunny.obj");

	glEnable(GL_DEPTH_TEST);
	initShaders();
	for(auto && o: rObjects){
		o.geometry.initVBO();
	}
}

void Geometry::drawModel()
{

	glBindBuffer(GL_ARRAY_BUFFER, this->vertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);

	glDrawElements(GL_TRIANGLES, this->faces.size() * 3, GL_UNSIGNED_INT, 0);
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
	glm::mat4 matT = glm::translate(glm::mat4(1.0), objCenter);
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), (0. / 180.) * M_PI, glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 matRz = glm::rotate<float>(glm::mat4(1.0), angleRad, glm::vec3(0.0, 0.0, 1.0));
	modelingMatrix = matT ;
	// modelingMatrix = matT * matRz * matRy * matRx;

	// Set the active program and the values of its uniform variables
	glUseProgram(gProgram[activeProgramIndex]);
	glUniformMatrix4fv(projectionMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc[activeProgramIndex], 1, glm::value_ptr(eyePos));

	// Draw the scene
	for(auto && o: rObjects){
		o.geometry.drawModel();
	}

	angle += 0.5;
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		//glShadeModel(GL_SMOOTH);
		activeProgramIndex = 0;
	}
	else if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		//glShadeModel(GL_SMOOTH);
		activeProgramIndex = 1;
	}
	else if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		//glShadeModel(GL_FLAT);
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

void setViewingMatrix()
{
	// view matrix
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), (eyeRotY / 180.) * M_PI, glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (eyeRotX / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 matR = matRy * matRx;

	// use original because we don't reset the rotation to 0 after rotating. (so that it doesn't accumulate)
	auto origEyePos = glm::vec3(0.0, 0.0, 0.0);
	glm::mat4 matIT = glm::translate(glm::mat4(1.0), -orbitCenter);
	glm::mat4 matT = glm::translate(glm::mat4(1.0), orbitCenter);
	auto newEyePos4 = matT * matR * matIT * glm::vec4(origEyePos, 1.0);

	glm::vec3 newEyePos = glm::vec3(newEyePos4.x, newEyePos4.y, newEyePos4.z);

	eyePos = newEyePos;

	// Set the viewing matrix
	viewingMatrix = glm::lookAt(newEyePos, orbitCenter, glm::vec3(0.0f, 1.0f, 0.0f));
}

void reshape(GLFWwindow *window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

	// glMatrixMode(GL_PROJECTION);
	// glLoadIdentity();
	// glOrtho(-10, 10, -10, 10, -10, 10);
	// gluPerspective(45, 1, 1, 100);

	// Use perspective projection

	float fovyRad = (float)(45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, w / (float)h, 1.0f, 100.0f);

	// Assume default camera position and orientation (camera is at
	// (0, 0, 0) with looking at -z direction and its up vector pointing
	// at +y direction)

	setViewingMatrix();

	// glMatrixMode(GL_MODELVIEW);
	// glLoadIdentity();
}

int lastX = 0;
int lastY = 0;

void mouseMove(GLFWwindow* window, double xpos, double ypos)
{
	float mult = 0.5;
	if(lastX == 0 && lastY == 0){
		lastX = xpos;
		lastY = ypos;
		return;
	}
	float deltaX = xpos - lastX;
	float deltaY = ypos - lastY;
	lastX = xpos;
	lastY = ypos;

	eyeRotX += -deltaX*mult;
	eyeRotY += -deltaY*mult;
	eyeRotX = eyeRotX > 360 ? eyeRotX - 360 : eyeRotX;
	eyeRotX = eyeRotX < 0 ? eyeRotX + 360 : eyeRotX;
	//clamp
	eyeRotY = glm::clamp(eyeRotY, -85.f, 85.f);
	setViewingMatrix();
}

void mouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwSetCursorPosCallback(window, mouseMove);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		lastX = 0;
		lastY = 0;
		glfwSetCursorPosCallback(window, NULL);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
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
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	int width = 640, height = 480;
	window = glfwCreateWindow(width, height, "Simple Example", NULL, NULL);

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

	char rendererInfo[512] = { 0 };
	strcpy(rendererInfo, (const char*)glGetString(GL_RENDERER));
	strcat(rendererInfo, " - ");
	strcat(rendererInfo, (const char*)glGetString(GL_VERSION));
	glfwSetWindowTitle(window, rendererInfo);

	init();

	glfwSetKeyCallback(window, keyboard);
	glfwSetMouseButtonCallback(window, mouseButton);
	glfwSetWindowSizeCallback(window, reshape);

	reshape(window, width, height); // need to call this once ourselves
	mainLoop(window); // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

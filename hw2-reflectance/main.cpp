#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
	GLuint vao;
	void initVBO();
	GLuint vertexAttribBuffer, indexBuffer;
	int vertexDataSizeInBytes, normalDataSizeInBytes;
	glm::mat4 modelMatrix = glm::mat4(1.0f);
};

// hold textures?
class Material{
	public:
	string name; 
};

class Program{
	public:
	GLuint program;
	string name;
	GLuint vertexShader, fragmentShader;
	map<string, GLuint> uniforms;
};

map<string, Program> programs;

class RenderObject{
	public:
	Program* program;
	Geometry geometry;
	string name;
	map<string, float> props;
	glm::vec3 position;
	void calculateModelMatrix();
	void drawModel();
};

vector<RenderObject> rObjects;
RenderObject& getRenderObject(const string& name){
	for(auto& obj: rObjects){
		if(obj.name == name){
			return obj;
		}
	}
	throw "RenderObject not found";
}

bool ParseObj(const string& fileName, const string& name)
{
	fstream myfile;

	rObjects.emplace_back();
	auto &obj = rObjects.back();
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
///< [in]  Name of the shader file
///< [out] The contents of the file
bool ReadDataFromFile(const string& fileName, string& data){
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

Program& initShader(string name, string vert, string frag){
	// Create the program
	GLuint prog = glCreateProgram();

	// Create shaders
	GLuint vs = createVS(vert.c_str());
	GLuint fs = createFS(frag.c_str());

	// Attach the shaders to the program
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	// Link the program
	glLinkProgram(prog);
	GLint status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed for: "+name << endl;
		exit(-1);
	}

	auto p = Program();
	p.program = prog;
	p.name = name;
	p.fragmentShader = fs;
	p.vertexShader = vs;
	p.uniforms["modelingMatrix"] = glGetUniformLocation(prog, "modelingMatrix");
	p.uniforms["viewingMatrix"] = glGetUniformLocation(prog, "viewingMatrix");
	p.uniforms["projectionMatrix"] = glGetUniformLocation(prog, "projectionMatrix");
	p.uniforms["eyePos"] = glGetUniformLocation(prog, "eyePos");
	programs[name] = p;
	return programs[name];
}

void initShaders(){
	auto &skybox = initShader("skybox", "skyv.glsl", "skyf.glsl");
	skybox.uniforms["skybox"] = glGetUniformLocation(skybox.program, "skybox");
	auto &arm = initShader("arm", "vert.glsl", "frag.glsl");
}

void Geometry::initVBO()
{
	GLuint &vao = this->vao;
	glGenVertexArrays(1, &vao);
	assert(vao > 0);
	glBindVertexArray(this->vao);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// assert(glGetError() == GL_NONE);

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

class Image{
	public:
	string name;
	int width, height, channels;
	unsigned char *data;
	Image(int width, int height, int channels, unsigned char *data, string name): width(width), height(height), channels(channels), data(data), name(name){}
	Image(){}
};

class ImgTexture{
	public:
	GLuint textureId;
};

map<string, Image> images;
map<string, ImgTexture> textures;

void loadTexture(Image& img){
	textures[img.name] = ImgTexture();
	ImgTexture &t = textures[img.name];

	glGenTextures(1, &t.textureId);
	glBindTexture(GL_TEXTURE_2D, t.textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if(img.channels == 3){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}else{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void flipImageHorizontally(unsigned char* imageData, int width, int height, int numChannels) {
    int rowStride = width * numChannels;
    unsigned char* tempRow = new unsigned char[rowStride];

    for (int y = 0; y < height / 2; y++) {
        memcpy(tempRow, &imageData[y * rowStride], rowStride);
        memcpy(&imageData[y * rowStride], &imageData[(height - y - 1) * rowStride], rowStride);
        memcpy(&imageData[(height - y - 1) * rowStride], tempRow, rowStride);
    }

    delete[] tempRow;
}

void loadCubemap(vector<string> names){
	textures["cubemap"] = ImgTexture();
	ImgTexture &t = textures["cubemap"];

	glGenTextures(1, &t.textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, t.textureId);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (int i = 0; i < names.size(); ++i){
		Image &img = images[names[i]];
		if (i == 0 || i == 1 || i == 4 || i == 5) {
			// flipImageHorizontally(img.data, img.width, img.height, img.channels);
		}		
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void genRandomImage(int width, int height){
	unsigned char *image = new unsigned char[width * height * 3];
	for (int i = 0; i < width * height; ++i){
		image[i] = rand() % 256;
		image[i+1] = 0;
		image[i+2] = 0;
	}
	images["random"] = Image(width, height, 3, image, "random");
	loadTexture(images["random"]);
}

void readImage(const char* path, string name, bool loadAsTex = true){
	int w,h,c;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *image = stbi_load(path, &w, &h, &c, 0);
	if (!image){
		std::cout << "Failed to load image" << std::endl;
	}else{
		images[name] = Image(w, h, c, image, name);
		if(loadAsTex)
		loadTexture(images[name]);
	}
}
void readSkybox(string path, string ext = "jpg"){
	readImage((path + "right." + ext).c_str(), "right");
	readImage((path + "left." + ext).c_str(), "left");
	readImage((path + "top." + ext).c_str(), "top");
	readImage((path + "bottom." + ext).c_str(), "bottom");
	readImage((path + "back." + ext).c_str(), "back");
	readImage((path + "front." + ext).c_str(), "front");

	loadCubemap({"right", "left", "bottom", "top", "front", "back"});
}
class SkyBox: public Geometry{
	public:
	void draw();
	void init();
};

void SkyBox::draw(){
	auto program = &programs["skybox"];
	glDepthFunc(GL_LEQUAL);
	glDepthRange(1, 1);
	glUseProgram(program->program);
	glUniformMatrix4fv(program->uniforms["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(program->uniforms["viewingMatrix"], 1, GL_FALSE, glm::value_ptr(viewingMatrix));

	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ::textures["cubemap"].textureId);

	glDrawElements(GL_TRIANGLES, this->faces.size() * 3, GL_UNSIGNED_INT, 0);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
}

void SkyBox::init(){
	this->vertices.push_back(Vertex(-1, -1, 0));
	this->vertices.push_back(Vertex(-1, 1, 0));
	this->vertices.push_back(Vertex(1, 1, 0));
	this->vertices.push_back(Vertex(1, -1, 0));
	this->normals.push_back(Normal(0, 0, 1));
	int vIndex[3] = {0, 1, 2};
	int nIndex[3] = {0, 0, 0};
	int tIndex[3] = {0, 0, 0};
	this->faces.push_back(Face(vIndex, tIndex, nIndex));
	vIndex[0] = 0;
	vIndex[1] = 2;
	vIndex[2] = 3;
	this->faces.push_back(Face(vIndex, tIndex, nIndex));
	this->initVBO();

	readSkybox("hw2_support_files/skybox_texture_test/", "jpg");
}
SkyBox skybox;
void init()
{
	ParseObj("hw2_support_files/obj/armadillo.obj", "armadillo");
	auto &armadillo = getRenderObject("armadillo");
	armadillo.position = objCenter;
	armadillo.program = &programs["arm"];
	//ParseObj("bunny.obj");
	// genRandomImage(300, 300);
	readImage("hw2_support_files/skybox_texture_abandoned_village/front.png", "skybox");

	glEnable(GL_DEPTH_TEST);
	initShaders();
	for(auto && o: rObjects){
		o.geometry.initVBO();
	}

	skybox.init();
}

void RenderObject::calculateModelMatrix(){
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), (0. / 180.) * M_PI, glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	this->geometry.modelMatrix = glm::translate(glm::mat4(1.0), this->position) * matRy * matRx;
}

void RenderObject::drawModel()
{

	// Set the active program and the values of its uniform variables
	glUseProgram(program->program);
	glUniformMatrix4fv(program->uniforms["modelingMatrix"], 1, GL_FALSE, glm::value_ptr(geometry.modelMatrix));
	glUniformMatrix4fv(program->uniforms["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(program->uniforms["viewingMatrix"], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniform3fv(program->uniforms["eyePos"], 1, glm::value_ptr(eyePos));

	glBindVertexArray(geometry.vao);
	glBindBuffer(GL_ARRAY_BUFFER, geometry.vertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer);

	glDrawElements(GL_TRIANGLES, geometry.faces.size() * 3, GL_UNSIGNED_INT, 0);
}


void display(){
	glClearColor(0, 0, 0, 1);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	skybox.draw();

	// Compute the modeling matrices
	for(auto && o: rObjects){
		o.calculateModelMatrix();
	}

	// Draw the scene
	for(auto && o: rObjects){
		o.drawModel();
	}

}

void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods){
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

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
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
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	// no need to alias reflections
	glfwWindowHint(GLFW_SAMPLES, 4);
	glEnable(GL_MULTISAMPLE);
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

	glDebugMessageCallback(debugCallback, NULL);

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

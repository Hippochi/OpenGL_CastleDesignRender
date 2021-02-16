//***************************************************************************
// GAME2012_Final_EnriqueDine.cpp 
// Student ID: 101244409 & 101264627
// Final Assignment submission.
//
// Description:
//Click run to see the results.
//*****************************************************************************

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 1.0f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)
#define XYZ_AXIS glm::vec3(1,1,1)


enum keyMasks {
	KEY_FORWARD =  0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,		
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;

// Texture variables.
GLuint hedgeTx, floorTx, wallTx, doorTx;
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(1.0f, 1.0f, 1.0f),	// Ambient colour.
	0.15f);							// Ambient strength.

DirectionalLight dLight(glm::vec3(0.0f, 0.0f, 0.0f), // Direction.
	glm::vec3(1.0f, 1.0f, 0.25f),  // Diffuse colour.
	1.0f);						  // Diffuse strength.

										//position		//sterngt			//colour			//strength?
PointLight pLights[2] = { { glm::vec3(-8.0f, 2.0f, -15.0f), 2.5f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f },
						  { glm::vec3(10.0f, 1.0f, -15.0f), 2.5f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f }};

SpotLight sLight(glm::vec3(1000.0f, 300.0f, -15.0f),	// Position.
	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse colour.
	1.0f,							// Diffuse strength.
	glm::vec3(0.0f, -1.0f, 0.0f),  // Direction.
	10.0f);

void timer(int);

//void resetView()
//{
//	position = glm::vec3(5.0f, 3.0f, 10.0f);
//	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
//	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
//	pitch = 0.0f;
//	yaw = -90.0f;
//	// View will now get set only in transformObject
//}
void resetView()
{
	position = glm::vec3(10.0f, 45.0f, -15.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = -89.0f;
	yaw = 0.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_castleCube;
Cube g_hedgeCube;
Cone g_towerCone(12);
Prism g_towerPrism(12);
Grid g_grid1(10, 10);
Grid g_mainGrid(30,30); // New UV scale parameter. Works with texture now.

void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("HEDGE.jpg", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &hedgeTx);
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);
	
	unsigned char* image2 = stbi_load("DERT.jpg", &width, &height, &bitDepth, 0);
	if (!image2) cout << "Unable to load file!" << endl;
	
	glGenTextures(1, &floorTx);
	glBindTexture(GL_TEXTURE_2D, floorTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);

	unsigned char* image3 = stbi_load("brick.jpg", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &wallTx);
	glBindTexture(GL_TEXTURE_2D, wallTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);

	unsigned char* image4 = stbi_load("door.jpg", &width, &height, &bitDepth, 0);
	if (!image4) cout << "Unable to load file!" << endl;

	glGenTextures(1, &doorTx);
	glBindTexture(GL_TEXTURE_2D, doorTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image4);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image4);

	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);

	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point lights.
	glUniform3f(glGetUniformLocation(program, "pLights[0].base.diffuseColour"), pLights[0].diffuseColour.x, pLights[0].diffuseColour.y, pLights[0].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].base.diffuseStrength"), pLights[0].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[0].position"), pLights[0].position.x, pLights[0].position.y, pLights[0].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].constant"), pLights[0].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[0].linear"), pLights[0].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[0].exponent"), pLights[0].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[1].base.diffuseColour"), pLights[1].diffuseColour.x, pLights[1].diffuseColour.y, pLights[1].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].base.diffuseStrength"), pLights[1].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[1].position"), pLights[1].position.x, pLights[1].position.y, pLights[1].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].constant"), pLights[1].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[1].linear"), pLights[1].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[1].exponent"), pLights[1].exponent);

	// Setting spot light.
	glUniform3f(glGetUniformLocation(program, "sLight.base.diffuseColour"), sLight.diffuseColour.x, sLight.diffuseColour.y, sLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "sLight.base.diffuseStrength"), sLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	glUniform3f(glGetUniformLocation(program, "sLight.direction"), sLight.direction.x, sLight.direction.y, sLight.direction.z);
	glUniform1f(glGetUniformLocation(program, "sLight.edge"), sLight.edgeRad);

	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

		ibo = 0;
		glGenBuffers(1, &ibo);
	
		points_vbo = 0;
		glGenBuffers(1, &points_vbo);

		colors_vbo = 0;
		glGenBuffers(1, &colors_vbo);

		uv_vbo = 0;
		glGenBuffers(1, &uv_vbo);

		normals_vbo = 0;
		glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.

	// Change shape data.
	g_towerPrism.SetMat(0.1, 16);
	g_mainGrid.SetMat(0.0, 16);

	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable smoothing.
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);

	// Enable face culling.
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);

	timer(0); 
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);
	
	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glBindVertexArray(vao);

	///////////////////////////////////////main Floor/////////////////////////////////////////////////////////
	glBindTexture(GL_TEXTURE_2D, floorTx);
	//g_mainGrid.ColorShape(1.0f, 1.0f, 1.0f);
	g_mainGrid.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainGrid.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, floorTx);
	//g_grid1.ColorShape(1.0f, 1.0f, 1.0f);
	g_grid1.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(-10.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_grid1.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, floorTx);
	//g_grid1.ColorShape(1.0f, 1.0f, 1.0f);
	g_grid1.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(-10.0f, 0.0f, -10.0f));
	glDrawElements(GL_TRIANGLES, g_grid1.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, floorTx);
	//g_grid1.ColorShape(1.0f, 1.0f, 1.0f);
	g_grid1.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(-10.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_grid1.NumIndices(), GL_UNSIGNED_SHORT, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////castle walls cubes///////////////////////////////////////////////
	for (int i = 0; i < 20; i++)
	{
		//right wall
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i*2.0f-10.0f), 0.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 2.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 4.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//left wall
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 0.0f, -30.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 2.0f, -30.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 4.0f, -30.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//crenelations
		//left
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 6.0f, -30.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.5f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 9.0f), 6.0f, -30.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 6.0f, -28.5f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.5f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 9.0f), 6.0f, -28.5f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//right
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 6.0f, -0.5f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.5f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 9.0f), 6.0f, -0.5f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 10.0f), 6.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.5f, 0.5f), XYZ_AXIS, 0, glm::vec3((i * 2.0f - 9.0f), 6.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	}

	for (int i = 0; i < 15; i++)
	{
		//back wall

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(28.0f, 0.0f, (-i * 2.0f-2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(28.0f, 2.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(28.0f, 4.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//front wall

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 4.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//crenelations
		//front
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 1.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 6.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 0.5f, 1.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 6.0f, (-i * 2.0f - 1.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 1.0f), XYZ_AXIS, 0, glm::vec3(-8.5f, 6.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 0.5f, 1.0f), XYZ_AXIS, 0, glm::vec3(-8.5f, 6.0f, (-i * 2.0f - 1.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//back
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 1.0f), XYZ_AXIS, 0, glm::vec3(29.5f, 6.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 0.5f, 1.0f), XYZ_AXIS, 0, glm::vec3(29.5f, 6.0f, (-i * 2.0f - 1.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 1.0f), XYZ_AXIS, 0, glm::vec3(28.0f, 6.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 0.5f, 1.0f), XYZ_AXIS, 0, glm::vec3(28.0f, 6.0f, (-i * 2.0f - 1.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	}

	for (int i = 0; i < 7; i++)
	{
		//front wall right part
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 0.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 2.0f, (-i * 2.0f - 2.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		//front wall left part
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 0.0f, (i * 2.0f - 30.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 2.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 2.0f, (i * 2.0f - 30.0f)));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

	//center floor
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 0.1f, 1.0f), XYZ_AXIS, 0, glm::vec3(-j* 1.0f + 10.0f, 0.0f, (i * 1.0f - 17.0f)));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	//stairs
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(0.4f, 0.2f, 1.0f), XYZ_AXIS, 0, glm::vec3(j * 0.4f - 12.0f, j * 0.2f, -i * 1.0f - 15.0f));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(0.4f, 0.2f, 1.0f), XYZ_AXIS, 0, glm::vec3(-j * 0.4f - 7.0f, j * 0.2f, -i * 1.0f - 15.0f));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			//stair walls
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(0.4f, 0.6f, 0.3f), XYZ_AXIS, 0, glm::vec3(j * 0.4f - 12.0f, j * 0.2f, i * 1.7f - 16.0f));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(0.4f, 0.6f, 0.3f), XYZ_AXIS, 0, glm::vec3(-j * 0.4f - 7.0f, j * 0.2f, i * 1.7f - 16.0f));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}

		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_castleCube.ColorShape(1.0f, 1.0f, 1.0f);
		g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.4f, 0.2f, 1.0f), XYZ_AXIS, 0, glm::vec3(-10.0f, 1.0f, -i * 1.0f - 15.0f));
		glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

	//door
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			glBindTexture(GL_TEXTURE_2D, doorTx);
			g_castleCube.ColorShape(0.7f, 0.4f, 0.0f);
			g_castleCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(0.2f, 1.0f, 1.0f), XZ_AXIS, 0, glm::vec3(-9.5f, j * 1.0f + 1.0f, -i * 1.0f - 15.0f));
			glDrawElements(GL_TRIANGLES, g_castleCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	


	///////////////////////////////////////////hedge cubes//////////////////////////////////////////////////

	//if you open any of these you will be cursed, unless you give 100% on this assignment in the next 25 hours
	//This is a joke, pls dont hate us, we did all of this manually

	for (int i = 0; i < 1; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -24.0f + i * 1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(1.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(1.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -10.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -24.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-5.0f, j, -6.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-5.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-5.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-3.0f, j, -6.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -16.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-3.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(1.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(3.0f, j, -22.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(3.0f, j, -25.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-1.0f, j, -12.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -12.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, j, -6.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, j, -25.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(7.0f, j, -21.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, j, -9.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, j, -22.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, j, -7.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((15.0f + i * 1.0f), j, -27.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((16.0f + i * 1.0f), j, -14.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((23.0f + i * 1.0f), j, -23.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}//1
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -2.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-3.0f, j, -15.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-1.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-1.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(1.0f, j, -3.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(2.0f, j, -3.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(2.0f, j, -19.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(2.0f, j, -24.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(2.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(3.0f, j, -11.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(3.0f, j, -19.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -7.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -21.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -8.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -22.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(5.0f, j, -26.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, j, -11.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, j, -15.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(8.0f, j, -3.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(8.0f, j, -27.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(20.0f, j, -16.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, j, -10.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(25.0f, j, -15.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(27.0f, j, -15.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-7.0f + i * 1.0f), j, -4.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-7.0f + i * 1.0f), j, -11.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-5.0f + i * 1.0f), j, -25.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-3.0f + i * 1.0f), j, -21.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((0.0f + i * 1.0f), j, -16.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((2.0f + i * 1.0f), j, -6.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((12.0f + i * 1.0f), j, -10.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((12.0f + i * 1.0f), j, -21.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((17.0f + i * 1.0f), j, -16.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((18.0f + i * 1.0f), j, -13.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((18.0f + i * 1.0f), j, -20.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((22.0f + i * 1.0f), j, -25.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((23.0f + i * 1.0f), j, -8.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((23.0f + i * 1.0f), j, -17.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((24.0f + i * 1.0f), j, -3.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((24.0f + i * 1.0f), j, -28.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}//2
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -21.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-2.0f, j, -17.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-2.0f, j, -25.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(0.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(0.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(1.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(7.0f, j, -10.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(11.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(13.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, j, -19.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, j, -23.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((11.0f + i * 1.0f), j, -8.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((11.0f + i * 1.0f), j, -23.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((22.0f + i * 1.0f), j, -6.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}// 3
	}

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -28.0f + i * 1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-6.0f, j, -19.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-6.0f, j, -24.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-6.0f, j, -9.0f + i * 1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -10.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(0.0f, j, -8.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(0.0f, j, -24.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(3.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f, j, -14.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(7.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(7.0f, j, -24.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);


			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, j, -10.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-5.0f + i * 1.0f), j, -23.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((10.0f + i * 1.0f), j, -6.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((10.0f + i * 1.0f), j, -11.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((10.0f + i * 1.0f), j, -20.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((10.0f + i * 1.0f), j, -25.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((11.0f + i * 1.0f), j, -4.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((11.0f + i * 1.0f), j, -27.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((18.0f + i * 1.0f), j, -15.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}// 4
	}

	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -22.0f + i * 1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-6.0f, j, -13.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(2.0f, j, -8.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(7.0f, j, -15.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(19.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(19.0f, j, -22.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-8.0f, j, -2.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-4.0f, j, -5.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(-2.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(21.0f, j, -6.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((19.0f + i * 1.0f), j, -4.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((19.0f + i * 1.0f), j, -27.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}
	}

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(9.0f, j, -3.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(9.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(17.0f, j, -3.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(17.0f, j, -20.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(21.0f, j, -17.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(26.0f, j, -5.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(26.0f, j, -18.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	for (int i = 0; i < 11; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(15.0f, j, -4.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(15.0f, j, -16.0f + i * -1.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j <= 2; j++)
		{

		}
	}

	for (int i = 0; i < 18; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((7.0f + i * 1.0f), j, -18.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}
	}

	for (int i = 0; i < 31; i++)
	{

		for (int j = 0; j <= 2; j++)
		{
			glBindTexture(GL_TEXTURE_2D, hedgeTx);
			g_hedgeCube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3((-6.0f + i * 1.0f), j, -13.0f));
			glDrawElements(GL_TRIANGLES, g_hedgeCube.NumIndices(), GL_UNSIGNED_SHORT, 0);

		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////castle towers///////////////////////////////////////////////
	//bottom right prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 0.0f, -3.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//bottom right cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 10.0f, -3.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//bottom left prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 0.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//bottom left cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 10.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//top left prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//top left cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 10.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//top right prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -3.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//top right cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 10.0f, -3.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//middle right prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 0.0f, -13.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//middle left prism
	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_towerPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 10.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_towerPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//middle left cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 10.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//middle right cone
	g_towerCone.ColorShape(1.0f, 0.0f, 0.0f);
	g_towerCone.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 5.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(-12.0f, 10.0f, -13.0f));
	glDrawElements(GL_TRIANGLES, g_towerCone.NumIndices(), GL_UNSIGNED_SHORT, 0);
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000/FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	case 'i':
		sLight.position.z -= 0.1; break;
	case 'j':
		sLight.position.x -= 0.1; break;
	case 'k':
		sLight.position.z += 0.1; break;
	case 'l':
		sLight.position.x += 0.1; break;
	case 'p':
		sLight.position.y += 0.1; break;
	case ';':
		sLight.position.y -= 0.1; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &hedgeTx);
	glDeleteTextures(1, &floorTx);
	glDeleteTextures(1, &wallTx);

}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("GAME2012_Finale_EnriqueDine");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.
	
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}

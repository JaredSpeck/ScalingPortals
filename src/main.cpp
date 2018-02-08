/**
 * CPE 471 Final Project
 * Scaling portals
 * Jared Speck
 * (Built with a provided template that implemented basic OpenGL initialization, window creation,
 * matrix stack definition, and .obj file shape loading)
 * Spring 2017
 */

#define GLEW_STATIC
#include <time.h>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GLSL.h"
#include "Program.h"
#include "PortalBox.h"
#include "MatrixStack.h"
#include "Shape.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

// WASD move speed (1-2 = slow, 2-5 = moderate, 6+ = sanik) (depends on number of objects rendering and processing power)
#define MOVE_SPEED 5
// Number of objects to render. 1/4 pyramids, 3/4 sneks (must be divisible by 4)
#define NUM_OBJECTS 16
// Portal cube's size (2.0-5.0 works best)
#define PORTAL_SIZE 2.0f


#define SCREEN_FRAME_BUF 0

static const int CIRCLE_NUM_VERTICES = 20;
static const int CYL_NUM_CIRCLES = 3;
static const int CYL_VERTEX_BUFFER_SIZE = 3 * (2 + CIRCLE_NUM_VERTICES * CYL_NUM_CIRCLES);

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> sceneShader, creatureShader, portalShader;
shared_ptr<Shape> pyramid;
shared_ptr<Shape> quad;
shared_ptr<Shape> sphere;
shared_ptr<Shape> cyl;
shared_ptr<PortalBox> pb;

float g_time = 0.0;
int g_width = 800;
int g_height = 800;
float sTheta;
float g_pitch;
float g_yaw;
float g_heightScale = 1.0f;
float g_widthScale = 1.0f;
int gMat = 1;
bool g_mouseCaptured = false;
vec3 g_gaze = vec3(0.0, 0.0, -100.0);
vec3 g_eyePos = vec3(0.0, 0.0, 10.0);
vec3 g_eyeU = vec3(1.0, 0.0, 0.0);
vec3 g_eyeV = vec3(0.0, 1.0, 0.0);
vec3 g_eyeW = vec3(0.0, 0.0, 1.0);

vec3 g_mirrorEyePos = vec3(0.0, 0.0, 0.0);
vec3 g_mirrorGaze = vec3(0.0, 0.0, -100.0);

// Array holding generated data for scene objects (X, Z, Y-Rotation)
float sceneArrangement[4 * NUM_OBJECTS] = { 0.0 };

// Cylinder stuff
float cylVertex[CYL_VERTEX_BUFFER_SIZE] = { 0 };
int cylIndex[6 * CIRCLE_NUM_VERTICES * CYL_NUM_CIRCLES] = { 0 };

// Forward declaration for useful functions
void SetMaterial(int i);
void DrawSnek(shared_ptr<MatrixStack>& M);
void DrawScene(int previewOption, glm::vec3& camPos, glm::vec3& camGaze);
void WarpEye(int entrySide, glm::vec3& leftEdge, glm::vec3& rightEdge, glm::vec3& portalNormal);


/*OpenGl callback functions */
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		gMat = (gMat + 1) % 4;
	}
	else if (g_mouseCaptured &&key == GLFW_KEY_W) {
		g_eyePos.x -= 0.1 * MOVE_SPEED * g_eyeW.x;
		g_eyePos.z -= 0.1 * MOVE_SPEED * g_eyeW.z;
	}
	else if (g_mouseCaptured &&key == GLFW_KEY_S) {
		g_eyePos.x += 0.1 * MOVE_SPEED * g_eyeW.x;
		g_eyePos.z += 0.1 * MOVE_SPEED * g_eyeW.z;
	}
	else if (g_mouseCaptured &&key == GLFW_KEY_A) {
		g_eyePos.x -= 0.1 * MOVE_SPEED * g_eyeU.x;
		g_eyePos.z -= 0.1 * MOVE_SPEED * g_eyeU.z;
	}
	else if (g_mouseCaptured && key == GLFW_KEY_D) {
		g_eyePos.x += 0.1 * MOVE_SPEED * g_eyeU.x;
		g_eyePos.z += 0.1 * MOVE_SPEED * g_eyeU.z;
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		g_mouseCaptured = !g_mouseCaptured;
		glfwSetInputMode(window, GLFW_CURSOR, g_mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		return;
	}

	g_gaze.x = -100 * cos(radians(g_pitch)) * cos(radians(90 + g_yaw)) + g_eyePos.x;
	g_gaze.y = -100 * sin(radians(g_pitch)) + g_eyePos.y;
	g_gaze.z = -100 * cos(radians(g_pitch)) * cos(radians(g_yaw)) + g_eyePos.z;

}
static void resize_callback(GLFWwindow *window, int width, int height) {
	g_width = width;
	g_height = height;
	glViewport(0, 0, width, height);
}
static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
	int x = (int)xpos;
	int y = (int)ypos;
	float newPitch, newYaw;
	float dPitch, dYaw;

	if (g_mouseCaptured) {
		newPitch = ((y - g_height / 2) % g_height) / (float)g_height * 360;
		newYaw = ((x - g_width / 2) % g_width) / (float)g_width * 360;

		// Calculate change in pitch and yaw
		dPitch = newPitch - g_pitch;
		dYaw = newYaw - g_yaw;

		// Cap the pitch angles
		if (g_pitch + dPitch > 80) {
			g_pitch = 80;
		}
		else if (g_pitch + dPitch < -80) {
			g_pitch = -80;
		}
		else {
			g_pitch += dPitch;
		}

		// Set yaw angles for 360 movement
		if (g_yaw + dYaw > 180) {
			g_yaw = g_yaw + dYaw - 360;
		}
		else if (g_yaw + dYaw < -180) {
			g_yaw = g_yaw + dYaw + 360;
		}
		else {
			g_yaw += dYaw;
		}

		g_gaze.x = -100 * cos(radians(g_pitch)) * cos(radians(90 + g_yaw)) + g_eyePos.x;
		g_gaze.y = -100 * sin(radians(g_pitch)) + g_eyePos.y;
		g_gaze.z = -100 * cos(radians(g_pitch)) * cos(radians(g_yaw)) + g_eyePos.z;

		// Set eye's coordinate frame basis vectors (for translation with WASD)
		g_eyeW = normalize(vec3(-1 * g_gaze.x, -1 * g_gaze.y, -1 * g_gaze.z));
		g_eyeU = normalize(cross(vec3(0.0, 1.0, 0.0), g_eyeW));
		g_eyeV = normalize(cross(g_eyeW, g_eyeU));

		//cout << "Pitch: " << g_pitch << " Yaw: " << g_yaw << endl;
	}
}


/* Portal callback functions */
// Warps the scene if one of the portals is entered
void WarpScene(int side) {
	//std::cout << "Warped with side " << side + 1 << " effect." << std::endl;
	switch (side) {
	case 0:
		g_widthScale *= 2.0f;
		break;
	case 1:
		g_widthScale *= 0.5f;
		break;
	case 2:
		g_heightScale *= 2.0f;
		break;
	case 3:
		g_heightScale *= 0.5f;
		break;
	default:
		break;
	}
}
// Checks if Camera is should be teleported by given side
void WarpEye(int entrySide, glm::vec3& leftEdge, glm::vec3& rightEdge, glm::vec3& portalNormal){
	float eyeDepth;
	float depthBound;

	// Check if XY plane or ZY plane
	if (portalNormal.x == 0.0) { // XY
		if (g_eyePos.x > leftEdge.x && g_eyePos.x < rightEdge.x) { // Check alignment with portal
			eyeDepth = (leftEdge.z - g_eyePos.z) / portalNormal.z;
			depthBound = (leftEdge.z + 2 * pb->boxScale * portalNormal.z) / portalNormal.z;
			if (eyeDepth > leftEdge.z && eyeDepth < depthBound) {
				g_eyePos.z += 4 * portalNormal.z * pb->boxScale;
				g_gaze.z += 4 * portalNormal.z * pb->boxScale;
				WarpScene(entrySide);
			}
		}
	}
	else { // ZY
		if (g_eyePos.z > leftEdge.z && g_eyePos.z < rightEdge.z) { // Check alignment with portal
			eyeDepth = (leftEdge.x - g_eyePos.x) / portalNormal.x;
			depthBound = (leftEdge.x + 2 * pb->boxScale * portalNormal.x) / portalNormal.x;
			if (eyeDepth > leftEdge.x && eyeDepth < depthBound) { // Check distance from plane
				g_eyePos.x += 4 * portalNormal.x * pb->boxScale;
				g_gaze.x += 4 * portalNormal.x * pb->boxScale;
				WarpScene(entrySide);
			}
		}
	}
}
// Draws the scene from the given position and gaze, applying a preview scaling if specified
void DrawScene(int previewOption, glm::vec3& camPos, glm::vec3& camGaze) {
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width / (float)height;
	int objIndex;
	float objX, objZ, objRot, objScale;

	float previewWidth = 1.0f;
	float previewHeight = 1.0f;

	switch (previewOption) {
	case -1:
		break;
	case 0:
		previewWidth = 2.0f;
		break;
	case 1:
		previewWidth = 0.5f;
		break;
	case 2:
		previewHeight = 2.0f;
		break;
	case 3:
		previewHeight = 0.5f;
		break;
	default:
		break;
	}

	// Create the matrix stacks
	auto P = make_shared<MatrixStack>();
	auto M = make_shared<MatrixStack>();

	// Set up projection and view matrices
	P->pushMatrix();
	P->perspective(45.0f, aspect, 0.01f, 100.0f);
	glm::mat4 viewMatrix = glm::lookAt(camPos, camGaze, vec3(0.0, 1.0, 0.0));

	sceneShader->bind();
	glUniformMatrix4fv(sceneShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(sceneShader->getUniform("V"), 1, GL_FALSE, value_ptr(viewMatrix));

	// Model matrices
	M->pushMatrix();
	M->loadIdentity();

	/* draw floor */
	M->pushMatrix();
	M->translate(vec3(0.0, -2.0, 0.0));
	M->rotate(radians(-90.0f), vec3(1.0, 0.0, 0.0));
	M->scale(vec3(100.0, 100.0, 1.0));
	SetMaterial((gMat + 1) % 4);
	glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	quad->draw(sceneShader);
	M->popMatrix();

	/* draw pyramids */
	SetMaterial((gMat) % 4);
	for (objIndex = 0; objIndex < NUM_OBJECTS / 4; ++objIndex) {
		objX = sceneArrangement[4 * objIndex];
		objZ = sceneArrangement[4 * objIndex + 1];
		objRot = sceneArrangement[4 * objIndex + 2];
		objScale = sceneArrangement[4 * objIndex + 3] + 10.0;

		M->pushMatrix();
		M->translate(vec3(objX, -1.75, objZ));
		M->rotate(objRot, vec3(0, 1, 0));
		M->scale(vec3(objScale * g_widthScale * previewWidth, objScale * g_heightScale * previewHeight, objScale * g_widthScale * previewWidth));
		glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		pyramid->draw(sceneShader);
		M->popMatrix();
	}
	sceneShader->unbind();

	creatureShader->bind();
	glUniformMatrix4fv(creatureShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(creatureShader->getUniform("V"), 1, GL_FALSE, value_ptr(viewMatrix));

	/* draw sneks */
	for (objIndex = NUM_OBJECTS / 4; objIndex < NUM_OBJECTS; ++objIndex) {
		objX = sceneArrangement[4 * objIndex];
		objZ = sceneArrangement[4 * objIndex + 1];
		objRot = sceneArrangement[4 * objIndex + 2];
		objScale = sceneArrangement[4 * objIndex + 3] - 0.5;

		M->pushMatrix();
		M->translate(vec3(objX, -1.75, objZ));
		M->rotate(objRot, vec3(0, 1, 0));
		M->scale(vec3(objScale * g_widthScale * previewWidth, objScale * g_heightScale * previewHeight, objScale * g_widthScale * previewWidth));
		DrawSnek(M);
		M->popMatrix();
	}
	creatureShader->unbind();
	/**/
	M->popMatrix();
	P->popMatrix();
}


/* Helper Functions */
// Snek drawing function
void DrawSnek(shared_ptr<MatrixStack>& M) {
	float objColor[3];

	// Mlem
	objColor[0] = 1.0f;
	objColor[1] = 0.25f;
	objColor[2] = 0.5f;
	glUniform3fv(creatureShader->getUniform("objColor"), 1, objColor);
	M->translate(vec3(0.0, 1.0, 0.0));
	M->pushMatrix();
	M->scale(vec3(0.1f + 0.1f * sin(3 * g_time), 0.01f, 0.05f));
	M->rotate(-0.6f, vec3(0.0, 0.0, 1.0));
	M->translate(vec3(1.6f, 0.0f, 0.0f));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();

	// Eyes
	objColor[0] = 0.25f;
	objColor[1] = 0.15f;
	objColor[2] = 0.0f;
	glUniform3fv(creatureShader->getUniform("objColor"), 1, objColor);
	M->pushMatrix();
	M->scale(vec3(0.06f + 0.005 * sin(3 * g_time), 0.06f + 0.005 * sin(3 * g_time), 0.06f + 0.005 * sin(3 * g_time)));
	// Left
	M->pushMatrix();
	M->translate(vec3(1.4f, 1.4f, -1.3f));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Right
	M->pushMatrix();
	M->translate(vec3(1.4f, 1.4f, 1.3f));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	M->popMatrix();
	//  Head
	objColor[0] = 1.0f;
	objColor[1] = 0.55f;
	objColor[2] = 0.0f;
	glUniform3fv(creatureShader->getUniform("objColor"), 1, objColor);
	M->translate(vec3(-0.05, 0.03, 0.0));
	M->rotate(radians(90.0), vec3(0.0, 1.0, 0.0));
	M->pushMatrix();
	M->scale(vec3(0.2, 0.15, 0.3));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Top neck
	M->translate(vec3(0.0, -0.25, -0.15));
	M->rotate(radians(75.0), vec3(1.0, 0.0, 0.0));
	M->pushMatrix();
	M->scale(vec3(0.45, 0.15, 0.45));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Mid neck
	M->translate(vec3(0.0, -0.15, 0.45));
	M->rotate(radians(30.0), vec3(1.0, 0.0, 0.0));
	M->pushMatrix();
	M->scale(vec3(0.2, 0.15, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Low neck
	M->rotate(radians(-120.0), vec3(1.0, 0.0, 0.0));
	M->translate(vec3(0.0, -0.3, -0.5));
	M->pushMatrix();
	M->scale(vec3(0.2, 0.15, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 1
	M->translate(vec3(0.0, 0.0, -0.35));
	M->rotate(radians(15.0), vec3(1.0, 0.0, 0.0));
	M->rotate(radians(60.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.35));
	M->pushMatrix();
	M->scale(vec3(0.2, 0.15, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 2
	M->translate(vec3(0.0, 0.0, -0.4));
	M->rotate(radians(-60.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.4));
	M->pushMatrix();
	M->scale(vec3(0.2, 0.15, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 3
	M->translate(vec3(0.0, 0.0, -0.4));
	M->rotate(radians(-60.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.4));
	M->pushMatrix();
	M->scale(vec3(0.17, 0.12, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 4
	M->translate(vec3(0.0, 0.0, -0.4));
	M->rotate(radians(45.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.4));
	M->pushMatrix();
	M->scale(vec3(0.15, 0.1, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 5
	M->translate(vec3(0.0, 0.0, -0.4));
	M->rotate(radians(60.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.4));
	M->pushMatrix();
	M->scale(vec3(0.13, 0.08, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
	// Body 5
	M->translate(vec3(0.0, 0.0, -0.4));
	M->rotate(radians(-45.0), vec3(0.0, 1.0, 0.0));
	M->translate(vec3(0.0, 0.0, -0.4));
	M->pushMatrix();
	M->scale(vec3(0.1, 0.05, 0.5));
	glUniformMatrix4fv(creatureShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(creatureShader);
	M->popMatrix();
}
// Sets shader materials for shading the scene
void SetMaterial(int i) {
	switch (i) {
	case 3: //shiny blue plastic
		glUniform3f(sceneShader->getUniform("MatAmb"), 0.02f, 0.04f, 0.2f);
		glUniform3f(sceneShader->getUniform("MatDif"), 0.0f, 0.16f, 0.9f);
		break;
	case 0: // flat grey
		glUniform3f(sceneShader->getUniform("MatAmb"), 0.13f, 0.13f, 0.14f);
		glUniform3f(sceneShader->getUniform("MatDif"), 0.3f, 0.3f, 0.4f);
		break;
	case 1: //brass
		glUniform3f(sceneShader->getUniform("MatAmb"), 0.3294f, 0.2235f, 0.02745f);
		glUniform3f(sceneShader->getUniform("MatDif"), 0.7804f, 0.5686f, 0.11373f);
		break;
	case 2: //copper
		glUniform3f(sceneShader->getUniform("MatAmb"), 0.1913f, 0.0735f, 0.0225f);
		glUniform3f(sceneShader->getUniform("MatDif"), 0.7038f, 0.27048f, 0.0828f);
		break;
	}
}
// Generates the geometry for a cylinder (used in the snek)
void generateCylinder() {
	int angleIndex;
	int heightIndex;
	int bufIndex = 0, centerIndex = 0;
	float angle = 0.0f;

	// Bottom center point
	cylVertex[bufIndex++] = 0.0f;
	cylVertex[bufIndex++] = 0.0f;
	cylVertex[bufIndex++] = -1.0f;
	// Top center point
	cylVertex[CYL_VERTEX_BUFFER_SIZE - 3] = 0.0f;
	cylVertex[CYL_VERTEX_BUFFER_SIZE - 2] = 0.0f;
	cylVertex[CYL_VERTEX_BUFFER_SIZE - 1] = 1.0f;

	// Generate vertices
	for (heightIndex = 0; heightIndex < CYL_NUM_CIRCLES; ++heightIndex) {
		for (angleIndex = 0; angleIndex < CIRCLE_NUM_VERTICES; ++angleIndex) {
			angle = radians(360.0) / CIRCLE_NUM_VERTICES * angleIndex;

			cylVertex[bufIndex++] = cos(angle);
			cylVertex[bufIndex++] = sin(angle);
			cylVertex[bufIndex++] = -1.0f + 2.0f / (CYL_NUM_CIRCLES - 1) * heightIndex;
		}
	}

	bufIndex = 0;
	// Generate indices
	for (heightIndex = 0; heightIndex < CYL_NUM_CIRCLES; ++heightIndex) {

		// Bottom face circle with upper half-wall of triangles
		if (heightIndex == 0) {
			centerIndex = 0;

			for (angleIndex = 0; angleIndex < CIRCLE_NUM_VERTICES; ++angleIndex) {
				// Fill in flat face
				cylIndex[bufIndex++] = centerIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;

				// Fill in upper half-wall
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;
				cylIndex[bufIndex++] = 1 + (heightIndex + 1) * CIRCLE_NUM_VERTICES + angleIndex;
			}
		}
		// Top face circle with lower half-wall of triangles
		else if (heightIndex == CYL_NUM_CIRCLES - 1) {
			centerIndex = CIRCLE_NUM_VERTICES * CYL_NUM_CIRCLES + 1;

			for (angleIndex = 0; angleIndex < CIRCLE_NUM_VERTICES; ++angleIndex) {
				// Fill in flat face
				cylIndex[bufIndex++] = centerIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;

				// Fill in lower half-wall
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;
				cylIndex[bufIndex++] = 1 + (heightIndex - 1) * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;
			}
		}
		// Interior circles with two attatched half-walls of triangles
		else {
			for (angleIndex = 0; angleIndex < CIRCLE_NUM_VERTICES; ++angleIndex) {
				// Fill in lower half-wall
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;
				cylIndex[bufIndex++] = 1 + (heightIndex - 1) * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;

				// Fill in upper half-wall
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + angleIndex;
				cylIndex[bufIndex++] = 1 + heightIndex * CIRCLE_NUM_VERTICES + (angleIndex + 1) % CIRCLE_NUM_VERTICES;
				cylIndex[bufIndex++] = 1 + (heightIndex + 1) * CIRCLE_NUM_VERTICES + angleIndex;
			}
		}
	}
}


// Initializes shaders and shapes
static void initGL()
{
	GLSL::checkVersion();
	int width, height, portalNdx;
	glfwGetFramebufferSize(window, &width, &height);

	int objIndex;
	float randX, randZ;

	sTheta = 0;
	g_pitch = 0.0;
	g_yaw = 90.0;
	// Set background color.
	glClearColor(.12f, .34f, .56f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize the obj mesh VBOs etc
	pyramid = make_shared<Shape>();
	pyramid->loadMesh(RESOURCE_DIR + "pyramid.obj");
	pyramid->resize();
	pyramid->init();

	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphere->resize();
	sphere->init();

	// Initialize the quad shape
	quad = make_shared<Shape>();
	quad->loadMesh(RESOURCE_DIR + "quad.obj");
	quad->resize();
	quad->init();

	// Initialize cylinder
	cyl = make_shared<Shape>();
	generateCylinder();
	cyl->loadData(cylVertex, CYL_VERTEX_BUFFER_SIZE, cylIndex, 6 * CIRCLE_NUM_VERTICES * CYL_NUM_CIRCLES);
	cyl->resize();
	cyl->init();

	// Initialize the GLSL program to render the obj
	sceneShader = make_shared<Program>();
	sceneShader->setVerbose(true);
	sceneShader->setShaderNames(RESOURCE_DIR + "scene_vert.glsl", RESOURCE_DIR + "scene_frag.glsl");
	sceneShader->init();
	sceneShader->addUniform("P");
	sceneShader->addUniform("V");
	sceneShader->addUniform("M");
	sceneShader->addUniform("MatAmb");
	sceneShader->addUniform("MatDif");
	sceneShader->addAttribute("vertPos");
	sceneShader->addAttribute("vertNor");

	creatureShader = make_shared<Program>();
	creatureShader->setVerbose(true);
	creatureShader->setShaderNames(RESOURCE_DIR + "creature_vert.glsl", RESOURCE_DIR + "creature_frag.glsl");
	creatureShader->init();
	creatureShader->addUniform("P");
	creatureShader->addUniform("M");
	creatureShader->addUniform("V");
	creatureShader->addUniform("time");
	creatureShader->addUniform("objColor");
	creatureShader->addAttribute("vertPos");
	creatureShader->addAttribute("vertNor");

	portalShader = make_shared<Program>();
	portalShader->setVerbose(true);
	portalShader->setShaderNames(RESOURCE_DIR + "portal_vert.glsl", RESOURCE_DIR + "portal_frag.glsl");
	portalShader->init();
	portalShader->addUniform("texBuf");
	portalShader->addUniform("portalNormal");
	portalShader->addUniform("boxScale");
	portalShader->addUniform("M");
	portalShader->addUniform("P");
	portalShader->addUniform("V");
	portalShader->addAttribute("vertPos");
	portalShader->addAttribute("vertNor");

	// Initialize the Portal Box (pass in center of box)
	pb = make_shared<PortalBox>(glm::vec3(0.0, 0.0, 0.0), window, DrawScene, WarpEye, PORTAL_SIZE);

	// Make offering to the RNG gods
	srand(time(NULL));
	// Set up randomly generated locations/rotation for scene objects
	for (objIndex = 0; objIndex < NUM_OBJECTS; ++objIndex) {
		randX = (float)rand() / RAND_MAX;
		randX = randX > 0.5f && randX < 0.6f ? randX * 5.0f * randX : randX;
		randX = randX < 0.5f && randX > 0.4f ? randX * randX : randX;
		sceneArrangement[4 * objIndex] = (randX - 0.5f) * 4 * NUM_OBJECTS;

		randZ = (float)rand() / RAND_MAX;
		randZ = randZ > 0.5f && randZ < 0.6f ? randZ * 5.0f * randZ : randZ;
		randZ = randZ < 0.5f && randZ > 0.4f ? randZ * randZ : randZ;
		sceneArrangement[4 * objIndex + 1] = (randZ - 0.5f) * 4 * NUM_OBJECTS;

		sceneArrangement[4 * objIndex + 2] = radians((float)rand() / RAND_MAX * 360);
		sceneArrangement[4 * objIndex + 3] = (float)rand() / RAND_MAX + 0.5;
		cerr << "Drawing with parameters: " << sceneArrangement[3 * objIndex] << ", " << sceneArrangement[3 * objIndex + 1] << ", " << sceneArrangement[3 * objIndex + 2] << ", " << sceneArrangement[4 * objIndex + 3] << endl;
	}

	cerr << "Made portal box" << endl;
}
// The render loop - this function is called repeatedly during the OGL program run
static void render()
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Clear screen for new render
	glBindFramebuffer(GL_FRAMEBUFFER, SCREEN_FRAME_BUF);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render the portalbox
	pb->Render(g_eyePos, g_gaze, portalShader);

	// Render scene for "player"
	glBindFramebuffer(GL_FRAMEBUFFER, SCREEN_FRAME_BUF);
	DrawScene(-1, g_eyePos, g_gaze);

	g_time += 0.05;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	cout << "Error callback set" << endl;
	// Initialize the library.
	if (!glfwInit()) {
		cout << "error initing lib." << endl;
		return -1;
	}
	cout << "Please specify the resource directory." << endl;
	//request the highest possible version of OGL - important for mac
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(g_width, g_height, "FBO test", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	//weird bootstrap of glGetError
	glGetError();
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	//set the window resize call back
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Set mouse cursor position callback
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	// Initialize scene. Note geometry initialized in init now
	initGL();

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

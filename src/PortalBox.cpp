
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "PortalBox.h"
#include "MatrixStack.h"
#include "Shape.h"

// set to 1 to show the back two faces, set to 0 for normal use(to show that portal ion optimization works)
#define SHOW_REVERSE 0

GLfloat portalVertexBufferData[] = {
    -1.0,      -1.0,      -1.0,
    -1.0,      -1.0,      1.0,
    -1.0,      1.0,      -1.0,
    -1.0,      1.0,      1.0,
    1.0,      -1.0,      -1.0,
    1.0,      -1.0,      1.0,
    1.0,      1.0,      -1.0,
    1.0,      1.0,      1.0,
};

// Triangles for faces in order: F, B, L, R
GLuint portalElementBufferData[] = {
    3, 1, 5,      3, 5, 7,
    6, 4, 0,      6, 0, 2,
    7, 5, 4,      7, 4, 6,
    2, 0, 1,      2, 1, 3,
};

void PortalBox::BindBuffers(int side) {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	// Bind and clear FBO
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufIds[side]);

	// Bind texture buffer
	glBindTexture(GL_TEXTURE_2D, texBufIds[side]);

	// Set texture buffer for framebuffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBufIds[side], 0);

	// Bind depth buffer
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufIds[side]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	// Set depth buffer for framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufIds[side]);

	// Draw output to color attatchment 0 (layout=0 on output of frag shader)
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Check for correct setup
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error setting up frame buffer - exiting" << std::endl;
		exit(0);
	}
}

PortalBox::PortalBox(glm::vec3& center, GLFWwindow *window, render_callback renderFunction, warp_callback warpFunction, float scale) {
    int sideNdx = 0, vertNdx = 0;

    // Make a local copy of the portal cube's center
    this->center = glm::vec3(center);
    this->window = window;
	this->renderCallback = renderFunction;
	this->warpCallback = warpFunction;
	this->boxScale = scale;

    // Generate framebuffer and related buffers for each side (4)
    glGenFramebuffers(NUM_SIDES, frameBufIds);
    glGenTextures(NUM_SIDES, texBufIds);
    glGenRenderbuffers(NUM_SIDES, depthBufIds);

	// Setup normals for portal sides
    normals.push_back(glm::vec3(0.0, 0.0, -1.0));
    normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    normals.push_back(glm::vec3(-1.0, 0.0, 0.0));
    normals.push_back(glm::vec3(1.0, 0.0, 0.0));

	// Translate/Scale Portal sides based on input
	while (vertNdx < 24) {
		portalVertexBufferData[vertNdx] = boxScale * portalVertexBufferData[vertNdx] + center.x;
		portalVertexBufferData[vertNdx + 1] = boxScale * portalVertexBufferData[vertNdx + 1] + center.y -2.0f + boxScale;
		portalVertexBufferData[vertNdx + 2] = boxScale * portalVertexBufferData[vertNdx + 2] + center.z;
		vertNdx += 3;
	}

    // Initialize the portal sides of the box
    this->front = std::make_shared<Portal>(portalVertexBufferData, 24, &portalElementBufferData[0], 6, normals[(int)FRONT]);
    this->back = std::make_shared<Portal>(portalVertexBufferData, 24, &portalElementBufferData[6], 6, normals[(int)BACK]);
    this->left = std::make_shared<Portal>(portalVertexBufferData, 24, &portalElementBufferData[12], 6, normals[(int)LEFT]);
    this->right = std::make_shared<Portal>(portalVertexBufferData, 24, &portalElementBufferData[18], 6, normals[(int)RIGHT]);

}

// Renders the sides of the portal directly visible to camera
void PortalBox::Render(glm::vec3& camPos, glm::vec3&camGaze, const  shared_ptr<Program> prog) {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width / (float)height;

	// Calculate center2camera vector components (for determining which sides to show);
	float relX = camPos.x - center.x;
	float relZ = camPos.z - center.z;
	glm::vec3 frontCam, frontGaze, backCam, backGaze, leftCam, leftGaze, rightCam, rightGaze;

	// Set up matrix stacks for portal drawing (not scene rendering as that is taken care of elsewhere)
	auto P = make_shared<MatrixStack>();
	auto M = make_shared<MatrixStack>();
	glm::mat4 viewMatrix = glm::lookAt(camPos, camGaze, vec3(0.0, 1.0, 0.0));

	P->pushMatrix();
	P->perspective(45.0f, aspect, 0.01f, 100.0f);
	M->pushMatrix();
	M->loadIdentity();

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

	// Render sides that can be seen by camera and check if warping is required
	if (SHOW_REVERSE ? relX < 0.0f : relX > 0.0f) { // Left vs Right
		leftCam = glm::vec3(center.x + boxScale * normals[(int)LEFT].x, center.y, center.z + boxScale * normals[(int)LEFT].z);
		leftGaze = glm::vec3(leftCam.x + boxScale * normals[(int)LEFT].x, leftCam.y, leftCam.z + boxScale * normals[(int)LEFT].z);
		BindBuffers((int)LEFT);
		renderCallback((int)LEFT, leftCam, leftGaze);

		warpCallback((int)LEFT, glm::vec3(center.x + boxScale * normals[(int)LEFT].x, center.y, center.z - boxScale), glm::vec3(center.x + boxScale * normals[(int)LEFT].x, center.y, center.z + boxScale), normals[(int)LEFT]);
	}
	else {
		rightCam = glm::vec3(center.x + boxScale * normals[(int)RIGHT].x, center.y, center.z + boxScale * normals[(int)RIGHT].z);
		rightGaze = glm::vec3(rightCam.x + boxScale * normals[(int)RIGHT].x, rightCam.y, rightCam.z + boxScale * normals[(int)RIGHT].z);
		BindBuffers((int)RIGHT);
		renderCallback((int)RIGHT, rightCam, rightGaze);

		warpCallback((int)RIGHT, glm::vec3(center.x + boxScale * normals[(int)RIGHT].x, center.y, center.z - boxScale), glm::vec3(center.x + boxScale * normals[(int)RIGHT].x, center.y, center.z + boxScale), normals[(int)RIGHT]);
	}

	if (SHOW_REVERSE ? relZ > 0.0f : relZ < 0.0f) { // Front vs Back
		backCam = glm::vec3(center.x + boxScale * normals[(int)BACK].x, center.y, center.z + boxScale * normals[(int)BACK].z);
		backGaze = glm::vec3(backCam.x + boxScale * normals[(int)BACK].x, backCam.y, backCam.z + boxScale * normals[(int)BACK].z);
		BindBuffers((int)BACK);
		renderCallback((int)BACK, backCam, backGaze);

		warpCallback((int)BACK, glm::vec3(center.x - boxScale, center.y, center.z + boxScale * normals[(int)BACK].x), glm::vec3(center.x + boxScale, center.y, center.z + boxScale * normals[(int)BACK].x), normals[(int)BACK]);
	}
	else {
		frontCam = glm::vec3(center.x + boxScale * normals[(int)FRONT].x, center.y, center.z + boxScale * normals[(int)FRONT].z);
		frontGaze = glm::vec3(frontCam.x + boxScale * normals[(int)FRONT].x, frontCam.y, frontCam.z + boxScale * normals[(int)FRONT].z);
		BindBuffers((int)FRONT);
		renderCallback((int)FRONT, frontCam, frontGaze);

		warpCallback((int)FRONT, glm::vec3(center.x - boxScale, center.y, center.z + boxScale * normals[(int)FRONT].x), glm::vec3(center.x + boxScale, center.y, center.z + boxScale * normals[(int)FRONT].x), normals[(int)FRONT]);
	}


	// Render portal in world
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	prog->bind();
	M->pushMatrix();
		glUniform1i(prog->getUniform("texBuf"), 0);
		glUniform1f(prog->getUniform("boxScale"), boxScale);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(viewMatrix));

		if (SHOW_REVERSE ? relZ < 0.0f : relZ > 0.0f) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texBufIds[(int)FRONT]);
			front->Draw(prog);
		}
		else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texBufIds[(int)BACK]);
			back->Draw(prog);
		}

		if (SHOW_REVERSE ? relX > 0.0f : relX < 0.0f) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texBufIds[(int)RIGHT]);
			right->Draw(prog);
		}
		else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texBufIds[(int)LEFT]);
			left->Draw(prog);
		}
	
	M->popMatrix();
	prog->unbind();

	P->popMatrix();
	M->popMatrix();
}

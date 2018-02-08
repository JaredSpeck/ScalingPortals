#ifndef PORTAL_BOX_H
#define PORTAL_BOX_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "Portal.h"

#define NUM_SIDES 4

typedef void(*render_callback)(int, glm::vec3&, glm::vec3&);
typedef void(*warp_callback)(int, glm::vec3&, glm::vec3&, glm::vec3&);

class PortalBox {

    enum PortalSide { FRONT = 0, BACK, LEFT, RIGHT };

    public:
	GLuint frameBufIds[NUM_SIDES];
	GLuint texBufIds[NUM_SIDES];
	GLuint depthBufIds[NUM_SIDES];
	float boxScale;

	PortalBox(glm::vec3&, GLFWwindow *, render_callback, warp_callback, float);
	void Render(glm::vec3&, glm::vec3&, const std::shared_ptr<Program>);

 private:
	 glm::vec3 center;
	 GLFWwindow *window;
	 render_callback renderCallback;
	 warp_callback warpCallback;

	 std::vector<glm::vec3> normals;

	 std::shared_ptr<Portal> front;
	 std::shared_ptr<Portal> back;
	 std::shared_ptr<Portal> left;
	 std::shared_ptr<Portal> right;

	 void BindBuffers(int);
};

#endif

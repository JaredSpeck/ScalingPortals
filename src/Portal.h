#ifndef PORTAL_H
#define PORTAL_H

#include <memory>
#include <vector>
#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "Program.h"

using namespace std;

class Portal {
	public:
		Portal(GLfloat *, int, GLuint *, int, glm::vec3&);
		void Draw(const shared_ptr<Program> prog) const;
	private:
		GLuint vaoId;
		GLuint posBufId;
		GLuint eleBufId;

		std::vector<GLuint> eleBuf;
		std::vector<GLfloat> posBuf;
        glm::vec3 normal;
};

#endif 

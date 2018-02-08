
#include "GLSL.h"
#include "Portal.h"

Portal::Portal(GLfloat *newPosBuf, int numPositions, GLuint *newEleBuf, int numElements, glm::vec3& normal) {
    // Make local copy of normal
    this->normal = glm::vec3(normal);
    
    posBuf.assign(newPosBuf, newPosBuf + numPositions);
        eleBuf.assign(newEleBuf, newEleBuf + numElements);
        
        // Initialize and bind the vertex array object
        glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    // Send the position array to the GPU
    glGenBuffers(1, &posBufId);
    glBindBuffer(GL_ARRAY_BUFFER, posBufId);
    glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(GLfloat), &posBuf[0], GL_STATIC_DRAW);
        
        // Send the element array to the GPU
        glGenBuffers(1, &eleBufId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size() * sizeof(GLuint), &eleBuf[0], GL_STATIC_DRAW);

    // Unbind the arrays
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Portal::Draw(const shared_ptr<Program> prog) const {
    int h_pos;

    //Bind portal VAO
    glBindVertexArray(vaoId);

    // Bind position buffer
    h_pos = prog->getAttribute("vertPos");
    GLSL::enableVertexAttribArray(h_pos);
    glBindBuffer(GL_ARRAY_BUFFER, posBufId);
    glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    // Bind element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufId);

    // Pass in normal
    glUniform3fv(prog->getUniform("portalNormal"), 1, value_ptr(normal));

    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void *)0);
        
    // Disable and unbind
    GLSL::disableVertexAttribArray(h_pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


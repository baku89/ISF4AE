#include "QuadVao.h"

namespace {
static const struct {
    float x, y;
} quadVertices[4] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
}  // namespace

namespace OGL {

QuadVao::QuadVao() {
    glGenBuffers(1, &this->quad);
    glBindBuffer(GL_ARRAY_BUFFER, this->quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    glGenVertexArrays(1, &this->ID);
    glBindVertexArray(this->ID);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->quad);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quadVertices[0]),
                          nullptr);
}

void QuadVao::render() {
    glBindVertexArray(this->ID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

QuadVao::~QuadVao() {
    if (this->ID) {
        glDeleteBuffers(1, &this->quad);
        glDeleteVertexArrays(1, &this->ID);
    }
}
}  // namespace OGL

#pragma once

#include <glad/glad.h>

namespace OGL {
class Texture {
   public:
    ~Texture();

    void allocate(GLsizei width, GLsizei height, GLenum format, GLenum pixelType);
    void bind();
    void unbind();
    GLuint getID();

   private:
    GLuint ID = 0;
    GLsizei width = 0;
    GLsizei height = 0;
    GLenum format = 0;
    GLenum pixelType = 0;
};

}  // namespace OGL
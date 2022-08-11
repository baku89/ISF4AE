#pragma once

#include <glad/glad.h>

namespace OGL {
class QuadVao {
   public:
    QuadVao();
    ~QuadVao();

    void render();

   private:
    GLuint ID = 0, quad = 0;
};

}  // namespace OGL
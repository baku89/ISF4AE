
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace OGL {

class GlobalContext {
   public:
    bool initialized = false;
    GlobalContext();
    ~GlobalContext();
    void bind();

   private:
    GLFWwindow *window;
};

}  // namespace OGL
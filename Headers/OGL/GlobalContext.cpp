#include "GlobalContext.h"
#include "Debug.h"

namespace OGL {

GlobalContext::GlobalContext() {
    if (!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(640, 480, "", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return;
    }

    // Setup Shader
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        FX_LOG("Failed to initialize GLAD");
        return;
    }

    // On succeeded
    this->window = window;
    this->initialized = true;
}

void GlobalContext::bind() {
    glfwMakeContextCurrent(window);
}

GlobalContext::~GlobalContext() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

}  // namespace OGL

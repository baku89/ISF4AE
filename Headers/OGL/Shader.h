#pragma once

#include <glad/glad.h>
#include <string>

#include "Debug.h"

namespace OGL {

class Shader {
public:
    
    Shader(const char *code, GLenum type): ID(0) {
        
        this->ID = glCreateShader(type);
        glShaderSource(this->ID, 1, &code, NULL);
        glCompileShader(this->ID);
        
        GLint success;
        glGetShaderiv(this->ID, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLchar infoLog[1024];
            glGetShaderInfoLog(this->ID, 1024, NULL, infoLog);
            
            std::string typeStr = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
            
            FX_LOG("Shader Complilation error (" << typeStr << "): " << infoLog);
            
            this->ID = 0;
        }
    }
    
    ~Shader() {
        glDeleteShader(this->ID);
    }
    
    GLuint getID() {
        return this->ID;
    }
    
private:
    GLuint ID;
};



} // namespace OGL

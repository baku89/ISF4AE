#pragma once

#include <glad/glad.h>
#include <string>

#include "Debug.h"

namespace OGL {

class Shader {
public:
    
    Shader(const char *code, GLenum type) {
        
        this->ID = glCreateShader(type);
        glShaderSource(this->ID, 1, &code, NULL);
        glCompileShader(this->ID);
        
        glGetShaderiv(this->ID, GL_COMPILE_STATUS, &this->success);
        
        if (!this->success) {
            std::string infoLog = this->getInfoLog();
            std::string typeStr = type == GL_FRAGMENT_SHADER ? "Fragment" : "Vertex";
            FX_LOG(typeStr << " shader " << infoLog);
        }
    }
    
    ~Shader() {
        glDeleteShader(this->ID);
    }

    bool isSucceed() {
        return this->success;
    }
    
    std::string getInfoLog() {
        if (this->success) {
            return "";
        }
        
        // TOOD: Avoid hard-coding 256 (= the maximum length of out_data->return_msg)
        GLchar infoLog[256];
        glGetShaderInfoLog(this->ID, 256, NULL, infoLog);
        
        return std::string(infoLog);
    }

    GLuint getID() {
        return this->ID;
    }
    
private:
    GLuint ID = 0;
    GLint success = false;
};



} // namespace OGL

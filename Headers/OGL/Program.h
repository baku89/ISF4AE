#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Debug.h"

#include "Shader.h"

namespace OGL {

class Program {
public:
    Program(OGL::Shader *vertex, OGL::Shader *fragment): ID(0) {
        
        // shader Program
        this->ID = glCreateProgram();
        glAttachShader(this->ID, vertex->getID());
        glAttachShader(this->ID, fragment->getID());
        glLinkProgram(this->ID);
        
        GLint success;
        glGetShaderiv(this->ID, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLchar infoLog[1024];
            glGetShaderInfoLog(this->ID, 1024, NULL, infoLog);
            
            FX_LOG("Program Linling error: " << infoLog);
            
            this->ID = 0;
        }
    }
    
    ~Program() {
        glDeleteProgram(this->ID);
    }
    
    bool isSucceed() {
        return this->ID != 0;
    }

    void bind() {
        glUseProgram(this->ID);
    }
    void unbind() {
        glUseProgram(0);
    }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
    }

    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(this->ID, name.c_str()), value);
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const {
        glUniform2f(glGetUniformLocation(this->ID, name.c_str()), x, y);
    }
    

    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(this->ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) {
        glUniform4f(glGetUniformLocation(this->ID, name.c_str()), x, y, z, w);
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setTexture(const std::string name, OGL::Texture *tex,
                    GLint index) {
        if (tex) {
            glActiveTexture(GL_TEXTURE0 + index);
            tex->bind();

            GLuint location = glGetUniformLocation(this->ID, name.c_str());
            glUniform1i(location, index);
        }
    }

private:
    GLuint ID;
};
}  // namespace OGL

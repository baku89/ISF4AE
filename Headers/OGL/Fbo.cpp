#include "Fbo.h"

namespace OGL {

Fbo::Fbo() : texture() {
}

Fbo::~Fbo() {
    if (this->ID) {
        glDeleteFramebuffers(1, &this->ID);
    }
    this->texture.~Texture();
    if (this->multisampledFbo) {
        glDeleteFramebuffers(1, &this->multisampledFbo);
    }
    if (this->multisampledTexture) {
        glDeleteTextures(1, &this->multisampledTexture);
    }
}

void Fbo::allocate(GLsizei width, GLsizei height, GLenum format, GLenum pixelType, int numSamples) {
    bool configChanged = this->width != width || this->height != height;
    configChanged |= this->pixelType != pixelType;
    configChanged |= this->numSamples != numSamples;

    this->width = width;
    this->height = height;
    this->format = format;
    this->pixelType = pixelType;
    this->numSamples = numSamples;

    if (configChanged) {
        glDeleteTextures(1, &this->ID);
        this->ID = 0;
    }

    if (this->ID == 0) {
        glGenFramebuffers(1, &this->ID);

        this->texture.allocate(width, height, format, pixelType);

        // // Bind to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, this->ID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               this->texture.getID(), 0);

        // Initialize multisampled FBO/texture
        if (this->numSamples > 0) {
            // Geneate FBo
            glGenFramebuffers(1, &this->multisampledFbo);
            glBindFramebuffer(GL_FRAMEBUFFER, this->multisampledFbo);

            // Generate multisampled texture
            glGenTextures(1, &this->multisampledTexture);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->multisampledTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->numSamples, format,
                                    this->width, this->height, GL_TRUE);

            // Bind the texture to FBo
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                                   this->multisampledTexture, 0);
        }
    }
}

void Fbo::bind() {
    GLuint fbo = this->numSamples > 0 ? this->multisampledFbo : this->ID;

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, this->width, this->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Fbo::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Texture* Fbo::getTexture() {
    if (this->numSamples > 0) {
        // Bind the multisampled FBO for reading
        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampledFbo);
        // Bind the normal FBO for drawing
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ID);
        // Blit the multisampled FBO to the normal FBO
        glBlitFramebuffer(0, 0, this->width, this->height,
                          0, 0, this->width, this->height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    return &this->texture;
}

void Fbo::readToPixels(void* pixels) {
    if (this->numSamples > 0) {
        // Bind the multisampled FBO for reading
        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampledFbo);
        // Bind the normal FBO for drawing
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ID);
        // Blit the multisampled FBO to the normal FBO
        glBlitFramebuffer(0, 0, this->width, this->height,
                          0, 0, this->width, this->height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // Bing the normal FBO for reading
    glBindFramebuffer(GL_FRAMEBUFFER, this->ID);
    // Read Ppxels
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, this->width, this->height, this->format, this->pixelType, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace OGL

#pragma once

#ifndef EffetRenderData_h
#define EffetRenderData_h

#include <OpenGL/gl3.h>
#include <GLUT/GLUT.h>

//general includes
#include <memory>

//typedefs
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;

struct EffectRenderData
{
	
	EffectRenderData() :
		initialized(false),
		width(0), height(0),
		frameBuffer(0),
		outputFrameTexture(0),
		program(0),
		vao(0),
		quad(0)
	{
		
	};
	
	virtual ~EffectRenderData() {
		//local OpenGL resource un-loading
		if (outputFrameTexture)
			glDeleteTextures(1, &outputFrameTexture);
		
		//common OpenGL resource unloading
		if (program)
			glDeleteProgram(program);
		
		if (swizzleProgram)
			glDeleteProgram(swizzleProgram);
		
		//release framebuffer resources
		if (frameBuffer)
			glDeleteFramebuffers(1, &frameBuffer);
		
		if (vao) {
			glDeleteBuffers(1, &quad);
			glDeleteVertexArrays(1, &vao);
		}
	}
	
	
	bool		initialized;
	
	u_int16		width, height;
	
	GLuint		frameBuffer;
	GLuint		outputFrameTexture;
	GLuint		program, swizzleProgram;
	GLuint		vao;
	GLuint		quad;

};

typedef std::shared_ptr<EffectRenderData> EffectRenderDataPtr;

#endif /* EffetRenderData_h */

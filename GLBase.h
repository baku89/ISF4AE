//
//  GLBase.hpp
//  ISF4AE
//
//  Created by Baku Hashimoto on 2017/07/05.
//
//
#pragma once

#ifndef GL_BASE_H
#define GL_BASE_H

#include <OpenGL/gl3.h>
#include <GLUT/GLUT.h>

#include <string>

namespace AESDK_OpenGL {
	
	void makeCurrentFlush(CGLContextObj newRC);
	NSOpenGLContext* createNSContext(NSOpenGLContext* pNSShare, CGLContextObj& rc);
	
	class SaveRestoreOGLContext {
	public:
		SaveRestoreOGLContext();
		~SaveRestoreOGLContext();
		
	private:
	#ifdef AE_OS_MAC
		CGLContextObj    o_RC;
		NSOpenGLContext* pNSOpenGLContext_;
	#endif
		
		SaveRestoreOGLContext(const SaveRestoreOGLContext &);
		SaveRestoreOGLContext &operator=(const SaveRestoreOGLContext &);
	};
	
};

#endif // GL_BASE_H

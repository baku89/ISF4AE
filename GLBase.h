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

//OS specific includes
#ifdef AE_OS_WIN
    // mambo jambo with stdint
    #define _STDINT
    #define WIN32_LEAN_AND_MEAN
    #define DVACORE_MATCH_MAC_STDINT_ON_WINDOWS
    #include <stdint.h>
    #ifndef INT64_MAX
        #define INT64_MAX       LLONG_MAX
    #endif
        #ifndef INTMAX_MAX
        #define INTMAX_MAX       INT64_MAX
    #endif

    #include <windows.h>
    #include <stdlib.h>

    #if (_MSC_VER >= 1400)
        #define THREAD_LOCAL __declspec(thread)
    #endif
#else
    #define THREAD_LOCAL __thread
#endif

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

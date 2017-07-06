//
//  GLBase.cpp
//  ISF4AE
//
//  Created by Baku Hashimoto on 2017/07/05.
//
//

#include "GLBase.h"

namespace AESDK_OpenGL {
	
	class ScopedAutoreleasePool {
		
	public:
		
		ScopedAutoreleasePool() : pool_(nil)
		{
			pool_ = [[NSAutoreleasePool alloc] init];
		}
		
		~ScopedAutoreleasePool()
		{
			[pool_ drain];
			pool_ = nil;
		}
		
	private:
		NSAutoreleasePool* pool_;
	};

	void makeCurrentFlush(CGLContextObj newRC) {
		// workaround for WATSONBUG 1541765
		// Windows automagically inserts a flush call on the old context before actually
		// changing over to the new context
		// Here we're doing the same thing on the mac manually, it also fixes the memory leak in the above bug.
		if (newRC != CGLGetCurrentContext())
		{
			GLint nCurrentFBO;
			GLenum status;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&nCurrentFBO));
			if (nCurrentFBO == 0
				|| (GL_FRAMEBUFFER_COMPLETE== (status = glCheckFramebufferStatus(GL_FRAMEBUFFER)))) {
				glFlush();
			}
			/*const CGLError cglError = */CGLSetCurrentContext(newRC);
		}
	}
	
	NSOpenGLContext* createNSContext(NSOpenGLContext* pNSShare, CGLContextObj& rc) {
		
		// Cocoa requires that a AutorelasePool be current for its Garbage Collection purposes
		// See NSAutoreleasePool API reference
		ScopedAutoreleasePool pool;
		
		NSOpenGLPixelFormatAttribute    aAttribs[64];
		int nIndex= -1;
		
		//aAttribs[++nIndex] = NSOpenGLPFAMPSafe;
		aAttribs[++nIndex] = NSOpenGLPFAClosestPolicy;
		aAttribs[++nIndex] = NSOpenGLPFANoRecovery;
		
		// color
		aAttribs[++nIndex] = NSOpenGLPFAAlphaSize;
		aAttribs[++nIndex] = 0;
		aAttribs[++nIndex] = NSOpenGLPFAColorSize;
		aAttribs[++nIndex] = 24;
		
		// stencil + depth
		aAttribs[++nIndex] = NSOpenGLPFAStencilSize;
		aAttribs[++nIndex] = 0;
		aAttribs[++nIndex] = NSOpenGLPFADepthSize;
		aAttribs[++nIndex] = 0;
		
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
		aAttribs[++nIndex] = NSOpenGLPFAOpenGLProfile;
		//aAttribs[++nIndex] = NSOpenGLProfileVersion3_2Core;
		aAttribs[++nIndex] = NSOpenGLProfileVersionLegacy;
#elif MAC_OS_X_VERSION_MAX_ALLOWED >= 101000
		aAttribs[++nIndex] = NSOpenGLPFAOpenGLProfile;
		aAttribs[++nIndex] = NSOpenGLProfileVersion4_1Core;
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
		
		aAttribs[++nIndex] = static_cast<NSOpenGLPixelFormatAttribute>(0);
		
		// create the context from the pixel format
		NSOpenGLContext* oContext = NULL;
		
		NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:aAttribs];
		
		oContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:pNSShare];
		rc = reinterpret_cast<CGLContextObj> ([oContext CGLContextObj]);
		
		[format release];
		return oContext;
	}
	

	/*
	 * SaveRestoreOGLContext
	 */

	SaveRestoreOGLContext::SaveRestoreOGLContext() {
#ifdef AE_OS_MAC
		ScopedAutoreleasePool pool;
		pNSOpenGLContext_ = [NSOpenGLContext currentContext];
		o_RC = CGLGetCurrentContext();
#endif
	}

	SaveRestoreOGLContext::~SaveRestoreOGLContext() {
#ifdef AE_OS_MAC
		ScopedAutoreleasePool pool;
		if (pNSOpenGLContext_ != [NSOpenGLContext currentContext] && pNSOpenGLContext_)
		{
			[pNSOpenGLContext_ makeCurrentContext];
		}
		makeCurrentFlush(o_RC);
#endif
	}
	

} // namespace ends

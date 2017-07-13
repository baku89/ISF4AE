#pragma once

#ifndef FILTER_H
#define FILTER_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;

#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#ifdef AE_OS_WIN
	#define STRNCPY(dest, src, len) strncpy_s(dest, src, len)
#else
	#define STRNCPY(dest, src, len) strncpy(dest, src, len)
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"


#include "GLSLCanvas_Strings.h"

#include <OpenGL/gl3.h>
#include <GLUT/GLUT.h>

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1

/* Parameter defaults */

#define RESTRICT_BOUNDS		0

#define FRAGPATH_MAX_LEN	512

enum {
	FILTER_INPUT = 0,
	FILTER_USE_LAYER_TIME_DISK_ID,
	FILTER_TIME_DISK_ID,
	FILTER_MOUSE,
	FILTER_SHOW_ERROR_DISK_ID,
	FILTER_NUM_PARAMS
};

enum {
	MOUSE_DISK_ID = 1
};


typedef struct EffectRenderData
{
	
	EffectRenderData() :
		initialized(false),
		width(0), height(0),
		frameBuffer(0),
		beforeSwizzleTexture(0),
		outputFrameTexture(0),
		program(0)
	{
	};
	
	virtual ~EffectRenderData() {
		//local OpenGL resource un-loading
		if (beforeSwizzleTexture)
			glDeleteTextures(1, &beforeSwizzleTexture);
		
		if (outputFrameTexture)
			glDeleteTextures(1, &outputFrameTexture);
		
		//common OpenGL resource unloading
		if (program)
			glDeleteProgram(program);
		
		//release framebuffer resources
		if (frameBuffer)
			glDeleteFramebuffers(1, &frameBuffer);
	}
	
	A_Boolean	flat;
	A_Boolean	initialized;
	
	u_int16		width, height;
	
	GLuint		frameBuffer;
	GLuint		beforeSwizzleTexture;
	GLuint		outputFrameTexture;
	GLuint		program;
	
	A_char      fragPath[FRAGPATH_MAX_LEN + 1];
	
} EffectRenderData;

//typedef struct {
//	A_Boolean	flat;
//	A_char		fragPath[FRAGPATH_MAX_LEN + 1];
//} FlatSeqData;


#ifdef __cplusplus
	extern "C" {
#endif

DllExport	PF_Err 
EntryPointFunc(	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra) ;

#ifdef __cplusplus
}
#endif

#endif // FILTER

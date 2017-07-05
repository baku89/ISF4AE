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

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
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

enum {
	FILTER_INPUT = 0,
	FILTER_MOUSE,
	FILTER_NUM_PARAMS
};

enum {
	MOUSE_DISK_ID = 1
};

#ifdef __cplusplus
	extern "C" {
#endif
		
typedef struct FilterInfo {
	PF_Fixed mouseX;
	PF_Fixed mouseY;
} FilterInfo, *FilterInfoP, *FilterInfoH;

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

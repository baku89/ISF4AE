#pragma once

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
#include "AEGP_SuiteHandler.h"

#include "OGL.h"
#include <VVISF/VVISF.hpp>
#include "ISF4AEScene.hpp"


#include <unordered_map>

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1

/* Parameter defaults */

#define ARB_REFCON          (void*)0xDEADBEEFDEADBEEF
#define ISFCODE_MAX_LEN	8192

enum {
    Param_Input = 0,
    Param_ISF,
    Param_Edit,
    Param_Save,
    Param_UseLayerTime,
    Param_Time,
    Param_UserOffset
    /*
     ISF parameters continues.
     As AESDK cannot change a type of parameter dynamically,
     this plug-in adapts a strategy to add all types parameters of ISF in order of UserParamType for each ISF input,
     then toggles their visibilities and only leaves an active type the in UpdateParamsUI.
     Use the formula shwon below to calculate the parameter index:
     index = userParamIndex * NumUserParamType + userParamType
     -- though in most cases, it'd be better to use the function getIndexForUserParam().
     */
};

// A subset of ISFValType supported by this plug-in
 enum UserParamType {
     UserParamType_None = -1,
    UserParamType_Bool = 0,
    UserParamType_Long,
    UserParamType_Float,
    UserParamType_Point2D,
    UserParamType_Color,
    UserParamType_Image,
    NumUserParamType
};

 static const uint32_t NumUserParams = 16;

 static const uint32_t NumParams = Param_UserOffset + NumUserParams * NumUserParamType;

typedef struct {
    VVISF::ISF4AESceneRef scene;
    std::string           status;
    std::string           errorLog;
} SceneDesc;

typedef struct {
    AEGP_PluginID       aegpId;
    OGL::GlobalContext  *context;
    VVISF::ISF4AESceneRef  defaultScene, ae2glScene, gl2aeScene;
    std::unordered_map<std::string, SceneDesc*> *scenes;
} GlobalData;

typedef struct {
    A_char code[ISFCODE_MAX_LEN];
} ParamArbIsf;

struct ParamInfo {
    VVISF::ISF4AEScene *scene;
    VVGL::Size outSize;
    VVGL::Size inputImageSizes[NumUserParams];
};

// Implemented in ISF4AE_UtilFunc.cpp
PF_ParamIndex getIndexForUserParam(PF_ParamIndex index, PF_ParamIndex type);
UserParamType getUserParamTypeForISFValType(VVISF::ISFValType type);
SceneDesc* getCompiledSceneDesc(GlobalData *globalData, A_char *code);
VVGL::GLBufferRef createRGBATexWithBitdepth(VVGL::Size &size, short format);

// Implemented in ISF4AE_ArbHandler.cpp
PF_Err
CreateDefaultArb(
    PF_InData            *in_data,
    PF_OutData           *out_data,
    PF_ArbitraryH        *dephault);

PF_Err
ArbCopy(
    PF_InData            *in_data,
    PF_OutData           *out_data,
    const PF_ArbitraryH  *srcP,
    PF_ArbitraryH        *dstP);

PF_Err
ArbCompare(
    PF_InData                *in_data,
    PF_OutData                *out_data,
    const PF_ArbitraryH        *a_arbP,
    const PF_ArbitraryH        *b_arbP,
    PF_ArbCompareResult        *resultP);


extern "C" {

DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data,
                            PF_OutData *out_data, PF_ParamDef *params[],
                            PF_LayerDef *output, void *extra);
}

#pragma once

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;

#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096

#define PF_DEEP_COLOR_AWARE \
  1  // make sure we get 16bpc pixels;
     // AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include "entry.h"

#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "String_Utils.h"

#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include <unordered_map>

#include <VVISF.hpp>

#include "ISF4AEScene.hpp"
#include "WeakMap.hpp"

#include "Config.h"

// Magic numbers
#define ARB_REFCON (void*)0xDEADBEEFDEADBEEF
#define PI 3.14159265358979323846

#ifdef AE_OS_MAC
#define DEFAULT_ISF_DIRECTORY "/Library/Graphics/ISF"
#endif

// Parameter indices
enum {
  Param_Input = 0,
  Param_ISFGroupStart,
  Param_ISF,
  Param_Edit,
  Param_Save,
  Param_ISFGroupEnd,
  Param_UseLayerTime,
  Param_Time,
  Param_UserOffset
  /*
   ISF parameters continues.
   As AESDK cannot change a type of parameter dynamically,
   this plug-in adapts a strategy to add all types parameters of ISF in order of UserParamType for each ISF input,
   then toggles their visibilities and only leaves an active type in UpdateParamsUI.
   Use the formula shwon below to calculate the parameter index:
   index = userParamIndex * NumUserParamType + userParamType
   -- though in most cases, it'd be better to use the function getIndexForUserParam().
   */
};

// A subset of ISFValType supported by this plug-in
enum UserParamType {
  UserParamType_Unsupported = -1,
  UserParamType_Bool = 0,
  UserParamType_Long,
  UserParamType_Float,
  UserParamType_Angle,
  UserParamType_Point2D,
  UserParamType_Color,
  UserParamType_Image,
  NumUserParamType
};

static const uint32_t NumUserParams = 16;

static const uint32_t NumParams = Param_UserOffset + NumUserParams * NumUserParamType;

struct SceneDesc {
  VVISF::ISF4AESceneRef scene;
  std::string status;
  std::string errorLog;
};

struct GlobalData {
  AEGP_PluginID aegpId;
  VVGL::GLContextRef context;
  VVGL::GLCPUToTexCopierRef uploader;
  VVGL::GLTexToCPUCopierRef downloader;
  VVISF::ISF4AESceneRef defaultScene, ae2glScene, gl2aeScene;
  std::shared_ptr<SceneDesc> notLoadedSceneDesc;
  WeakMap<std::string, SceneDesc>* scenes;
};

struct SequenceData {
  bool showISFOption;
};

struct ParamArbIsf {
  std::string name;
  std::shared_ptr<SceneDesc> desc;
};

struct ParamInfo {
  VVISF::ISF4AEScene* scene;
  VVGL::Size outSize;
  VVGL::Size inputImageSizes[NumUserParams];
};

// Implemented in ISF4AE_UtilFunc.cpp
PF_ParamIndex getIndexForUserParam(PF_ParamIndex index, UserParamType type);
UserParamType getUserParamTypeForISFAttr(const VVISF::ISFAttrRef input);
PF_Fixed getDefaultForAngleInput(VVISF::ISFAttrRef input);
bool isISFAttrVisibleInECW(const VVISF::ISFAttrRef input);
std::shared_ptr<SceneDesc> getCompiledSceneDesc(GlobalData* globalData, const std::string& code);
PF_Err saveISF(PF_InData* in_data, PF_OutData* out_data);
VVGL::GLBufferRef createRGBATexWithBitdepth(const VVGL::Size& size, VVGL::GLContextRef context, short bitdepth);
VVGL::GLBufferRef createRGBACPUBufferWithBitdepthUsing(const VVGL::Size& inCPUBufferSizeInPixels,
                                                       const void* inCPUBackingPtr,
                                                       const VVGL::Size& inImageSizeInPixels,
                                                       const short bitdepth);
PF_Err uploadCPUBufferInSmartRender(GlobalData* globalData,
                                    PF_ProgPtr effectRef,
                                    PF_SmartRenderExtra* extra,
                                    A_long checkoutIndex,
                                    const VVGL::Size outImageSize,
                                    VVGL::GLBufferRef& outImage);
PF_Err renderISFToCPUBuffer(PF_InData* in_data,
                            PF_OutData* out_data,
                            ISF4AEScene& scene,
                            short bitdepth,
                            VVGL::Size& outSize,
                            VVGL::Size& pointScale,
                            VVGL::GLBufferRef* outBuffer);

// Implemented in ISF4AE_ArbHandler.cpp
PF_Err CreateDefaultArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbitraryH* dephault);

PF_Err HandleArbitrary(PF_InData* in_data,
                       PF_OutData* out_data,
                       PF_ParamDef* params[],
                       PF_LayerDef* output,
                       PF_ArbParamsExtra* extra);

// Implemented in ISF4AE_EventHandler.cpp
PF_Err HandleEvent(PF_InData* in_data,
                   PF_OutData* out_data,
                   PF_ParamDef* params[],
                   PF_LayerDef* output,
                   PF_EventExtra* event_extraP);

extern "C" {

DllExport PF_Err EffectMain(PF_Cmd cmd,
                            PF_InData* in_data,
                            PF_OutData* out_data,
                            PF_ParamDef* params[],
                            PF_LayerDef* output,
                            void* extra);
}

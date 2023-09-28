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

#ifdef _WIN32
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

using namespace std;

#ifdef _WIN32
//  global compilation flag configuring windows sdk headers
//  preventing inclusion of min and max macros clashing with <limits>
#define NOMINMAX 1
//  override byte to prevent clashes with <cstddef>
#define byte win_byte_override
#include <Windows.h>
//  Undefine min max macros so they won't collide with <limits> header content.
#undef min
#undef max
//  Undefine byte macros so it won't collide with <cstddef> header content.
#undef byte
#endif

// Magic numbers
#define ARB_REFCON (void*)0xDEADBEEFDEADBEEF
#define PI 3.14159265358979323846

#ifdef AE_OS_MAC
#define DEFAULT_ISF_DIRECTORY "/Library/Graphics/ISF"
#else
#define DEFAULT_ISF_DIRECTORY ".\\"
#endif

#define BUTTON_WIDTH 70
#define BUTTON_HEIGHT 16
#define BUTTON_MARGIN 10

// Parameter indices
enum {
  Param_Input = 0,
  Param_ISF,
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

// Parameter ID used in ParamSetup; they should not be changed over the plugin's versions.
enum ParamID {
  Input = 0,
  // ISFGroupStart,
  ISF = 2,
  // Edit,
  // Save,
  // ISFGroupEnd,
  UseLayerTime = 6,
  Time,
  UserOffset
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
  string status;
  string errorLog;
};

struct GlobalData {
  AEGP_PluginID aegpId;
  VVGL::GLContextRef context;
  VVGL::GLCPUToTexCopierRef uploader;
  VVGL::GLTexToCPUCopierRef downloader;
  // The UV gradient shader that is applied when no shaders loaded or failed to compile.
  VVISF::ISF4AESceneRef defaultScene;
  // For filling the gap between the format of OpenGL texture and After Effects' image buffer.
  VVISF::ISF4AESceneRef ae2glScene, gl2aeScene;
  shared_ptr<SceneDesc> notLoadedSceneDesc;
  // Caches shader program by using the code as a key.
  shared_ptr<WeakMap<string, SceneDesc>> scenes;
#ifndef _WIN32
  NSLock* lock;
#endif
};

struct SequenceData {
  bool showISFOption;
};

// The data that is initialized in SmartPreRender and passed to SmartRender.
struct PreRenderData {
  VVISF::ISF4AEScene* scene;
  VVGL::Size outSize;
  VVGL::Size inputImageSizes[NumUserParams];
};

// A struct for representing arbitrary parmaeter type that stores shader data.
// The reason why it contains the name is that there is a case that two parameters have the same code but different names. The SceneDesc is memoized by the code as a key.
struct ParamArbIsf {
  string name;
  shared_ptr<SceneDesc> desc;
};

#define ARB_ISF_FLAT_V1_MAGIC_NUMBER 0x01

struct ParamArbIsfFlatV1 {
  A_u_char magicNumber;  // Always should be set to ARB_ISF_FLAT_V1_MAGIC_NUMBER
  A_u_long version;
  A_u_long offsetName;
  A_u_long offsetFragCode;
  A_u_long offsetVertCode;
};

// Implemented in ISF4AE_UtilFunc.cpp
PF_ParamIndex getIndexForUserParam(PF_ParamIndex index, UserParamType type);
PF_ParamIndex getIdForUserParam(PF_ParamIndex index, UserParamType type);
UserParamType getUserParamTypeForISFAttr(const VVISF::ISFAttrRef input);
PF_Fixed getDefaultForAngleInput(VVISF::ISFAttrRef input);
bool isISFAttrVisibleInECW(const VVISF::ISFAttrRef input);
shared_ptr<SceneDesc> getCompiledSceneDesc(GlobalData* globalData, const string& fsCode, const string& vsCode);
PF_Err loadISF(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]);
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
PF_Err
renderISFToCPUBuffer(PF_InData* in_data, PF_OutData* out_data, ISF4AEScene& scene, short bitdepth, VVGL::Size& outSize, VVGL::Size& pointScale, VVGL::GLBufferRef* outBuffer);

// Implemented in ISF4AE_ArbHandler.cpp
PF_Err CreateDefaultArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbitraryH* dephault);

PF_Err HandleArbitrary(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, PF_ArbParamsExtra* extra);

// Implemented in ISF4AE_EventHandler.cpp
PF_Err HandleEvent(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, PF_EventExtra* event_extraP);

extern "C" {

DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, void* extra);
}

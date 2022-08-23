#include "ISF4AE.h"

PF_ParamIndex getIndexForUserParam(PF_ParamIndex index, UserParamType type) {
  return Param_UserOffset + index * NumUserParamType + (int)type;
}

UserParamType getUserParamTypeForISFAttr(const VVISF::ISFAttrRef input) {
  switch (input->type()) {
    case VVISF::ISFValType_Bool:
      return UserParamType_Bool;

    case VVISF::ISFValType_Long:
      return UserParamType_Long;

    case VVISF::ISFValType_Float:
      switch (input->unit()) {
        case VVISF::ISFValUnit_Angle:
          return UserParamType_Angle;
        default:
          return UserParamType_Float;
      }

    case VVISF::ISFValType_Point2D:
      return UserParamType_Point2D;

    case VVISF::ISFValType_Color:
      return UserParamType_Color;

    case VVISF::ISFValType_Image:
      return UserParamType_Image;

    default:
      return UserParamType_Unsupported;
  }
}

/**
 * Check if an ISF input should be promoted and visible in Effect Conrol Window
 */
bool isISFAttrVisibleInECW(const VVISF::ISFAttrRef input) {
  auto userParamType = getUserParamTypeForISFAttr(input);
  auto& name = input->name();

  bool isInputImage = name == "inputImage";
  bool isISF4AESpecialAttr = name.rfind("i4a_") == 0;

  if (userParamType == UserParamType_Unsupported || isInputImage || isISF4AESpecialAttr) {
    return false;
  }

  return true;
}

/**
 * Compile a shader and store to pool that maps code string to OGL::Program.
 * It will be called at UpdateParameterUI and PreRender.
 */
SceneDesc* getCompiledSceneDesc(GlobalData* globalData, A_char* code) {
  // Compile a shader at here to make sure to display the latest status
  auto& scenes = *globalData->scenes;

  if (scenes.find(code) != scenes.end()) {
    // Use cache
    return scenes[code];
  }

  // Compile new shader if not exists
  globalData->context->bind();

  auto scene = VVISF::CreateISF4AESceneRef();
  scene->setThrowExceptions(true);
  scene->setManualTime(true);

  try {
    auto doc = VVISF::CreateISFDocRefWith(code);
    scene->useDoc(doc);
    scene->compileProgramIfNecessary();

    auto errDict = scene->errDict();

    if (errDict.size() > 0) {
      VVISF::ISFErr err = VVISF::ISFErr(VVISF::ISFErrType_ErrorCompilingGLSL, "Shader Problem",
                                        "check error dict for more info", errDict);
      throw err;
    }

    auto* desc = new SceneDesc();
    desc->scene = scene;
    desc->status = "Compiled Successfully";

    scenes[code] = desc;

  } catch (VVISF::ISFErr isfErr) {
    // On Failed, format and save the risen error.
    auto* desc = new SceneDesc();
    desc->scene = globalData->defaultScene;
    desc->status = isfErr.getTypeString();
    desc->errorLog = "";

    for (auto err : isfErr.details) {
      if (err.first == "fragSrc" || err.first == "vertSrc")
        continue;
      desc->errorLog += "[" + err.first + "] " + err.second + "\n";
    }

    scenes[code] = desc;
  }

  return scenes[code];
}

VVGL::GLBufferRef createRGBATexWithBitdepth(const VVGL::Size& size, short bitdepth) {
  switch (bitdepth) {
    case 8:
      return VVGL::CreateRGBATex(size);
    case 16:
      return VVGL::CreateRGBAShortTex(size);
    case 32:
      return VVGL::CreateRGBAFloatTex(size);
  }
}

VVGL::GLBufferRef createRGBACPUBufferWithBitdepthUsing(const VVGL::Size& inCPUBufferSizeInPixels,
                                                       const void* inCPUBackingPtr,
                                                       const VVGL::Size& inImageSizeInPixels,
                                                       const short bitdepth) {
  switch (bitdepth) {
    case 8:
      return VVGL::CreateRGBACPUBufferUsing(inCPUBufferSizeInPixels, inCPUBackingPtr, inImageSizeInPixels, NULL, NULL);

    case 16:
      return VVGL::CreateRGBAShortCPUBufferUsing(inCPUBufferSizeInPixels, inCPUBackingPtr, inImageSizeInPixels, NULL,
                                                 NULL);

    case 32:
      return VVGL::CreateRGBAFloatCPUBufferUsing(inCPUBufferSizeInPixels, inCPUBackingPtr, inImageSizeInPixels, NULL,
                                                 NULL);
  }
}

#include "ISF4AE.h"

#include <regex>

#include "AEFX_SuiteHelper.h"

#include "SystemUtil.h"

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
SceneDesc* getCompiledSceneDesc(GlobalData* globalData, const std::string& code) {
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
    scene->useCode(code);

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

    // Pretty-format the risen error
    for (auto err : isfErr.details) {
      if (err.first == "fragSrc" || err.first == "vertSrc") {
        continue;
      }

      if (err.first == "fragErrLog" || err.first == "vertErrLog") {
        // Omit the text "ERROR: 0:"
        auto log = std::regex_replace(err.second, std::regex("ERROR: 0:"), "Line ");

        desc->errorLog += log + "\n";

      } else if (err.first == "jsonErrLog") {
        std::string str = err.second;
        std::smatch m;
        std::regex re(R"(\[.+\] parse error at line ([0-9]+).+?: (.+); last read.*)");
        if (std::regex_match(str, m, re)) {
          std::string line = m[1].str();
          std::string msg = m[2].str();
          desc->errorLog += "Line " + line + ": " + msg + "\n";
        } else {
          desc->errorLog += err.second;
        }
      } else {
        desc->errorLog += err.second;
      }
    }

    scenes[code] = desc;

  } catch (...) {
    auto* desc = new SceneDesc();
    desc->scene = globalData->defaultScene;
    desc->status = "Unknown Error";
    desc->errorLog = "";

    scenes[code] = desc;
  }

  return scenes[code];
}

PF_Err saveISF(PF_InData* in_data, PF_OutData* out_data) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  // Save the current shader
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));

  // Set name of an effect instance as default file name
  // https://ae-plugins.docsforadobe.dev/aegps/aegp-suites.html#streamrefs-and-effectrefs
  AEGP_EffectRefH effectH = NULL;
  AEGP_StreamRefH glslStreamH, effectStreamH;
  A_char effectName[AEGP_MAX_ITEM_NAME_SIZE] = "shader.frag";

  ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(globalData->aegpId, in_data->effect_ref, &effectH));

  ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(globalData->aegpId, effectH, Param_ISF, &glslStreamH));

  ERR(suites.DynamicStreamSuite4()->AEGP_GetNewParentStreamRef(globalData->aegpId, glslStreamH, &effectStreamH));

  ERR(suites.StreamSuite2()->AEGP_GetStreamName(effectStreamH, FALSE, effectName));

  // Then confirm a destination path and save it
  std::string dstPath = SystemUtil::saveFileDialog(std::string(effectName) + ".fs");

  if (!err && !dstPath.empty()) {
    PF_ParamDef paramIsf;
    AEFX_CLR_STRUCT(paramIsf);
    ERR(PF_CHECKOUT_PARAM(in_data, Param_ISF, in_data->current_time, in_data->time_step, in_data->time_scale,
                          &paramIsf));

    auto* isf = reinterpret_cast<ParamArbIsf*>(*paramIsf.u.arb_d.value);

    std::string isfCode = std::string(isf->code);

    if (isfCode.empty()) {
      auto& doc = *globalData->defaultScene->doc();
      isfCode = *doc.jsonSourceString() + *doc.fragShaderSource();
    }

    SystemUtil::writeTextFile(dstPath, isfCode);

    ERR2(PF_CHECKIN_PARAM(in_data, &paramIsf));
  }

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return err;
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

// Function to convert and copy string literals to A_UTF16Char.
// On Win: Pass the input directly to the output
// On Mac: All conversion happens through the CFString format
void copyConvertStringLiteralIntoUTF16(const wchar_t* inputString, A_UTF16Char* destination) {
#ifdef AE_OS_MAC
  int length = wcslen(inputString);
  CFRange range = {0, AEGP_MAX_PATH_SIZE};
  range.length = length;
  CFStringRef inputStringCFSR =
      CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(inputString),
                              length * sizeof(wchar_t), kCFStringEncodingUTF32LE, FALSE);
  CFStringGetBytes(inputStringCFSR, range, kCFStringEncodingUTF16, 0, FALSE, reinterpret_cast<UInt8*>(destination),
                   length * (sizeof(A_UTF16Char)), NULL);
  destination[length] = 0;  // Set NULL-terminator, since CFString calls don't set it
  CFRelease(inputStringCFSR);
#elif defined AE_OS_WIN
  size_t length = wcslen(inputString);
  wcscpy_s(reinterpret_cast<wchar_t*>(destination), length + 1, inputString);
#endif
}

#include "ISF4AE.h"

#include <regex>
#include <sstream>

#include "AEFX_SuiteHelper.h"

#include "AEUtil.h"
#include "Debug.h"
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
  if (code.empty()) {
    return globalData->notLoadedSceneDesc.get();
  }

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

    // Check if the number of inputs does not exceed the maximum
    if (scene->inputs().size() > NumUserParams) {
      std::map<std::string, std::string> errDict;

      std::stringstream ss;
      ss << "The number of inputs (" << scene->inputs().size() << ") exceeds the maximum number of supported inputs ("
         << NumUserParams << ")";

      errDict["ia4ErrLog"] = ss.str();

      auto err = ISFErr(ISFErrType_ErrorLoading, "Invalid ISF", "", errDict);
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

  PF_ParamDef paramIsf;
  AEFX_CLR_STRUCT(paramIsf);
  ERR(PF_CHECKOUT_PARAM(in_data, Param_ISF, in_data->current_time, in_data->time_step, in_data->time_scale, &paramIsf));

  auto* isf = reinterpret_cast<ParamArbIsf*>(*paramIsf.u.arb_d.value);

  // Set name of an effect instance as default file name
  std::string effectName = isf->name;

  // Then confirm a destination path and save it
  std::string dstPath = SystemUtil::saveFileDialog(effectName + ".fs");

  if (!err && !dstPath.empty()) {
    auto* desc = getCompiledSceneDesc(globalData, isf->code);
    std::string isfCode = desc->scene->getFragCode();

    SystemUtil::writeTextFile(dstPath, isfCode);
  }

  ERR2(PF_CHECKIN_PARAM(in_data, &paramIsf));

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
    default:
      throw std::invalid_argument("Invalid bitdepth");
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

    default:
      throw std::invalid_argument("Invalid bitdepth");
  }
}

/**
 * Renders ISF scene to CPU buffer. It's used at SmartRender() and DrawEvent(), and assuming image inputs are already
 * bounded by the callees.
 */
PF_Err renderISFToCPUBuffer(PF_InData* in_data,
                            PF_OutData* out_data,
                            ISF4AEScene& scene,
                            short bitdepth,
                            VVGL::Size& outSize,
                            VVGL::Size& pointScale,
                            VVGL::GLBufferRef* outBuffer) {
  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));

  // In After Effects, 16-bit pixel doesn't use the highest bit, and thus each channel ranges 0x0000 - 0x8000.
  // So after passing pixel buffer to GPU, it should be scaled by (0xffff / 0x8000) to normalize the luminance to
  // 0.0-1.0.
  VVISF::ISFVal multiplier16bit(VVISF::ISFValType_Float, bitdepth == 16 ? (65535.0f / 32768.0f) : 1.0f);
  globalData->ae2glScene->setValueForInputNamed(multiplier16bit, "multiplier16bit");
  globalData->gl2aeScene->setValueForInputNamed(multiplier16bit, "multiplier16bit");

  // Render ISF
  auto isfImage = createRGBATexWithBitdepth(outSize, bitdepth);
  {
    // Assign time-related variables
    double fps = in_data->time_scale / in_data->local_time_step;
    double time = 0;

    PF_Boolean useLayerTime = false;
    ERR(AEUtil::getCheckboxParam(in_data, out_data, Param_UseLayerTime, &useLayerTime));

    if (useLayerTime) {
      time = (double)in_data->current_time / in_data->time_scale;
    } else {
      ERR(AEUtil::getFloatSliderParam(in_data, out_data, Param_Time, &time));
    }

    scene.setRenderFrameIndex(time * fps);
    scene.setRenderTimeDelta(1.0 / fps);

    // Assign user-defined parameters
    PF_ParamIndex userParamIndex = 0;

    for (auto input : scene.inputs()) {
      if (!isISFAttrVisibleInECW(input)) {
        continue;
      }

      auto isfType = input->type();
      auto userParamType = getUserParamTypeForISFAttr(input);
      auto paramIndex = getIndexForUserParam(userParamIndex, userParamType);

      VVISF::ISFVal* val = nullptr;

      switch (userParamType) {
        case UserParamType_Bool: {
          PF_Boolean v = false;
          ERR(AEUtil::getCheckboxParam(in_data, out_data, paramIndex, &v));
          val = new VVISF::ISFVal(isfType, v);
          break;
        }
        case UserParamType_Long: {
          A_long v = 0;
          ERR(AEUtil::getPopupParam(in_data, out_data, paramIndex, &v));
          val = new VVISF::ISFVal(isfType, v - 1);  // current begins from 1
          break;
        }
        case UserParamType_Float: {
          A_FpLong v = 0.0;
          ERR(AEUtil::getFloatSliderParam(in_data, out_data, paramIndex, &v));

          if (input->unit() == VVISF::ISFValUnit_Length) {
            v /= outSize.width;
          } else if (input->unit() == VVISF::ISFValUnit_Percent) {
            v /= 100;
          }

          val = new VVISF::ISFVal(isfType, v);
          break;
        }
        case UserParamType_Angle: {
          A_FpLong v = 0.0;
          AEUtil::getAngleParam(in_data, out_data, paramIndex, &v);
          v = (-v + 90.0) * (PI / 180.0);
          val = new VVISF::ISFVal(isfType, v);
          break;
        }
        case UserParamType_Point2D: {
          A_FloatPoint point;
          ERR(AEUtil::getPointParam(in_data, out_data, paramIndex, &point));
          // Since the above getter returns a coordinate considering downsampling, it requries to be compensated
          // inversely to render an image for Custom Comp UI
          point.x *= pointScale.width;
          point.y *= pointScale.height;

          // Should be converted to normalized and vertically-flipped coordinate
          point.x = point.x / outSize.width;
          point.y = 1.0 - point.y / outSize.height;
          val = new VVISF::ISFVal(isfType, point.x, point.y);
          break;
        }
        case UserParamType_Color: {
          PF_PixelFloat color;
          ERR(AEUtil::getColorParam(in_data, out_data, paramIndex, &color));
          val = new VVISF::ISFVal(isfType, color.red, color.green, color.blue, color.alpha);
          break;
        }
        case UserParamType_Image:
          // Assumes the image has already bounded
          break;
        default:
          FX_LOG("Invalid ISFValType.");
          break;
      }

      if (val != nullptr) {
        scene.setValueForInputNamed(*val, input->name());
      }

      userParamIndex++;
    }  // End of for each ISF->inputs

    // Then, render it!
    scene.renderToBuffer(isfImage, outSize, time);
  }

  // Download the result of ISF
  auto& gl2aeScene = *globalData->gl2aeScene;

  gl2aeScene.setBufferForInputNamed(isfImage, "inputImage");

  auto outputImage = createRGBATexWithBitdepth(outSize, bitdepth);
  gl2aeScene.renderToBuffer(outputImage);

  (*outBuffer) = globalData->context->downloader->downloadTexToCPU(outputImage);

  // Release resources
  VVGL::GetGlobalBufferPool()->housekeeping();
  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return err;
}

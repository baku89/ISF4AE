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
        case VVISF::ISFValUnit_Direction:
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

PF_Fixed getDefaultForAngleInput(VVISF::ISFAttrRef input) {
  auto unit = input->unit();
  double rad = input->defaultVal().getDoubleVal();
  // If both min/max aren't specified, VVISF automatically set them to 0 and 1 respectively,
  // and sets the default to their median value, 0.5. But 0.5 is not a nice round number in radians,
  // so tries to set 0 degrees (in AE's rotery knobs UI) in such a case.
  if (rad == 0.5f) {
    rad = unit == VVISF::ISFValUnit_Direction ? (PI / 2.0) : 0.0;
  }

  if (unit == VVISF::ISFValUnit_Direction) {
    return FLOAT2FIX(-(rad * 180.0 / PI) + 90.0);
  } else {  // (unit == ISFValUnit_Angle)
    return FLOAT2FIX(-(rad * 180.0 / PI));
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
shared_ptr<SceneDesc> getCompiledSceneDesc(GlobalData* globalData, const string& fsCode, const string& vsCode) {
  if (fsCode.empty()) {
    return globalData->notLoadedSceneDesc;
  }

  string key = fsCode + "__ISF_VERTEX__" + vsCode;

  // Compile a shader at here to make sure to display the latest status
  auto& scenes = *globalData->scenes;

  if (scenes.has(key)) {
    // Use cache
    return scenes.get(key);
  }

  // Compile new shader if not exists
  auto scene = VVISF::CreateISF4AESceneRefUsing(globalData->context->newContextSharingMe());
  scene->setThrowExceptions(true);
  scene->setManualTime(true);

  auto desc = make_shared<SceneDesc>();

  try {
    scene->useCode(fsCode, vsCode);

    auto errDict = scene->errDict();
    if (errDict.size() > 0) {
      throw ISFErr(ISFErrType_ErrorCompilingGLSL, "Linking Error", "", errDict);
    }

    if (scene->program() == 0) {
      throw ISFErr(ISFErrType_Unknown, "Unknown Error", "", map<string, string>());
    }

    // Check if the number of inputs does not exceed the maximum
    if (scene->inputs().size() > NumUserParams) {
      map<string, string> errDict;

      stringstream ss;
      ss << "The number of inputs (" << scene->inputs().size() << ") exceeds the maximum number of supported inputs ("
         << NumUserParams << ")";

      errDict["ia4ErrLog"] = ss.str();

      auto err = ISFErr(ISFErrType_ErrorLoading, "Invalid ISF", "", errDict);
      throw err;
    }

    desc->scene = scene;
    desc->status = "Compiled Successfully";

  } catch (VVISF::ISFErr isfErr) {
    // On Failed, format and save the risen error.
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
        auto log = regex_replace(err.second, regex("ERROR: 0:"), "Line ");

        desc->errorLog += log + "\n";

      } else if (err.first == "jsonErrLog") {
        string str = err.second;
        smatch m;
        regex re(R"(\[.+\] parse error at line ([0-9]+).+?: (.+); last read.*)");
        if (regex_match(str, m, re)) {
          string line = m[1].str();
          string msg = m[2].str();
          desc->errorLog += "Line " + line + ": " + msg + "\n";
        } else {
          desc->errorLog += err.second;
        }
      } else {
        desc->errorLog += err.second;
      }
    }

  } catch (...) {
    auto* desc = new SceneDesc();
    desc->scene = globalData->defaultScene;
    desc->status = "Unknown Error";
    desc->errorLog = "";
  }

  scenes.set(key, desc);

  return desc;
}

PF_Err saveISF(PF_InData* in_data, PF_OutData* out_data) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  // Save the current shader
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_ParamDef paramIsf;
  AEFX_CLR_STRUCT(paramIsf);
  ERR(PF_CHECKOUT_PARAM(in_data, Param_ISF, in_data->current_time, in_data->time_step, in_data->time_scale, &paramIsf));

  auto* isf = reinterpret_cast<ParamArbIsf*>(*paramIsf.u.arb_d.value);

  string isfDirectory = "";
  ERR(AEUtil::getStringPersistentData(in_data, CONFIG_MATCH_NAME, "ISF Directory", DEFAULT_ISF_DIRECTORY,
                                      &isfDirectory));

  // Set name of an effect instance as default file name
  string effectName = isf->name;

  // Then confirm a destination path and save it
  string dstPath = SystemUtil::saveFileDialog(effectName + ".fs", isfDirectory, "Save ISF File");

  if (!err && !dstPath.empty()) {
    string fsCode = isf->desc->scene->getFragCode();

    SystemUtil::writeTextFile(dstPath, fsCode);

    auto& vsCode = *isf->desc->scene->doc()->vertShaderSource();
    if (!vsCode.empty()) {
      auto noExtPath = VVGL::StringByDeletingExtension(dstPath);
      SystemUtil::writeTextFile(noExtPath + ".vs", vsCode);
    }

    isfDirectory = getDirname(dstPath);
    ERR(AEUtil::setStringPersistentData(in_data, CONFIG_MATCH_NAME, "ISF Directory", isfDirectory));
  }

  ERR2(PF_CHECKIN_PARAM(in_data, &paramIsf));

  return err;
}

VVGL::GLBufferRef createRGBATexWithBitdepth(const VVGL::Size& size, VVGL::GLContextRef context, short bitdepth) {
  context->makeCurrentIfNotCurrent();

  VVGL::GLBufferPoolRef bp = VVGL::GetGlobalBufferPool();

  switch (bitdepth) {
    case 8:
      return VVGL::CreateRGBATex(size, true, bp);
    case 16:
      return VVGL::CreateRGBAShortTex(size, true, bp);
    case 32:
      return VVGL::CreateRGBAFloatTex(size, true, bp);
    default:
      throw invalid_argument("Invalid bitdepth");
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
      throw invalid_argument("Invalid bitdepth");
  }
}

PF_Err uploadCPUBufferInSmartRender(GlobalData* globalData,
                                    PF_ProgPtr effectRef,
                                    PF_SmartRenderExtra* extra,
                                    A_long checkoutIndex,
                                    const VVGL::Size outImageSize,
                                    VVGL::GLBufferRef& outImage) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  auto bitdepth = extra->input->bitdepth;
  auto pixelBytes = bitdepth * 4 / 8;

  PF_LayerDef* layerDef = nullptr;

  ERR(extra->cb->checkout_layer_pixels(effectRef, checkoutIndex, &layerDef));

  if (layerDef != nullptr) {
    // Stores the actual buffer size of images which has just done checkout-- affected by downsamples and cropping.
    VVGL::Size imageSize(layerDef->width, layerDef->height);

    VVGL::Size bufferSizeInPixel(layerDef->rowbytes / pixelBytes, imageSize.height);

    if (imageSize.width > outImageSize.width || imageSize.height > outImageSize.height) {
      // I dunno why, but this case seems to occur without any exception when AE tries to generate thumanil for project
      // pane.
      FX_LOG("the size of image being done checkout exceeds the original dimension.");
      return err;
    }

    VVGL::GLBufferRef imageAECPU =
        createRGBACPUBufferWithBitdepthUsing(bufferSizeInPixel, layerDef->data, imageSize, bitdepth);

    auto imageAE = globalData->uploader->uploadCPUToTex(imageAECPU);

    // Note that AE's inputImage is cropped by mask's region and smaller than ISF resolution.
    glBindTexture(GL_TEXTURE_2D, imageAE->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, 0);

    auto origin = VVISF::ISFVal(VVISF::ISFValType_Point2D, layerDef->origin_x, layerDef->origin_y);

    globalData->ae2glScene->setBufferForInputNamed(imageAE, "inputImage");
    globalData->ae2glScene->setValueForInputNamed(origin, "origin");

    outImage = createRGBATexWithBitdepth(outImageSize, globalData->context, bitdepth);

    globalData->ae2glScene->renderToBuffer(outImage);

    // Though ISF specs does not specify the wrap mode of texture, set it to CLAMP_TO_EDGE to match with online ISF
    // editor's behavior.
    glBindTexture(GL_TEXTURE_2D, outImage->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  ERR2(extra->cb->checkin_layer_pixels(effectRef, checkoutIndex));

  return err;
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
  auto isfImage = createRGBATexWithBitdepth(outSize, globalData->context, bitdepth);
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
          A_long index = 0;
          ERR(AEUtil::getPopupParam(in_data, out_data, paramIndex, &index));
          auto v = input->valArray()[index - 1];  // Index of popup UI begins from 1
          val = new VVISF::ISFVal(isfType, v);
          break;
        }
        case UserParamType_Float: {
          A_FpLong v = 0.0;
          ERR(AEUtil::getFloatSliderParam(in_data, out_data, paramIndex, &v));

          if (input->unit() == VVISF::ISFValUnit_Length) {
            v /= in_data->width;
          } else if (input->unit() == VVISF::ISFValUnit_Percent) {
            v /= 100;
          }

          val = new VVISF::ISFVal(isfType, v);
          break;
        }
        case UserParamType_Angle: {
          A_FpLong v = 0.0;
          AEUtil::getAngleParam(in_data, out_data, paramIndex, &v);
          if (input->unit() == VVISF::ISFValUnit_Direction) {
            v = (-v + 90.0) * (PI / 180.0);
          } else {  // unit == VVISF::ISFValUnit_Angle
            v = -v * (PI / 180.0);
          }
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
        input->setCurrentVal(*val);
      }

      userParamIndex++;
    }  // End of for each ISF->inputs

    // Then, render it!
    scene.renderToBuffer(isfImage, outSize, time);
  }

  // Download the result of ISF
  auto& gl2aeScene = *globalData->gl2aeScene;

  gl2aeScene.setBufferForInputNamed(isfImage, "inputImage");

  auto outputImage = createRGBATexWithBitdepth(outSize, globalData->context, bitdepth);
  gl2aeScene.renderToBuffer(outputImage);

  (*outBuffer) = globalData->downloader->downloadTexToCPU(outputImage);

  // Release resources
  VVGL::GetGlobalBufferPool()->housekeeping();
  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return err;
}

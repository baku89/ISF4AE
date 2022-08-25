#include "ISF4AE.h"

#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "Config.h"

#include "AEUtil.h"
#include "SystemUtil.h"

#include "Debug.h"
#include "MiscUtil.h"

#include <VVGL.hpp>
#include <iostream>

#define GL_SILENCE_DEPRECATION
#define PI 3.14159265358979323846

/**
 * Display a dialog describing the plug-in. Populate out_data>return_msg and After Effects will display it in a simple
 * modal dialog.
 */
static PF_Err About(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* isf = reinterpret_cast<ParamArbIsf*>(*params[Param_ISF]->u.arb_d.value);

  auto desc = getCompiledSceneDesc(globalData, isf->code);
  auto& doc = *desc->scene->doc();

  std::stringstream ss;

  ss << "ISF Information" << std::endl;
  ss << "Name: " << isf->name << std::endl;
  ss << "Description: " << doc.description() << std::endl;
  ss << "Credit: " << doc.credit() << std::endl;

  ss << std::endl;

  ss << "----------------" << std::endl;

  ss << std::endl;

  ss << "This effect is built with ISF4AE; Interactive Shader Format for After Effects ";
  ss << "(v" << MAJOR_VERSION << "." << MINOR_VERSION << ")." << std::endl;
  ss << "All of the source code is published at https://github.com/baku89/ISF4AE." << std::endl;

  PF_STRCPY(out_data->return_msg, ss.str().c_str());

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return PF_Err_NONE;
}

/**
 * Pops up shader errors on user clicked Option label of the effect
 */
static PF_Err PopDialog(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto seqData = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(in_data->sequence_data));

  seqData->showISFOption = !seqData->showISFOption;

  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_ISFGroupStart, seqData->showISFOption));
  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_ISFGroupEnd, seqData->showISFOption));

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);
  suites.HandleSuite1()->host_unlock_handle(in_data->sequence_data);

  return err;
}

/**
 * Set any required flags and PF_OutData fields (including out_data>my_version) to describe your plug-in’s behavior.
 */
static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION, BUILD_VERSION);

  out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_CUSTOM_UI | PF_OutFlag_I_DO_DIALOG |
                        PF_OutFlag_NON_PARAM_VARY | PF_OutFlag_SEND_UPDATE_PARAMS_UI | PF_OutFlag_CUSTOM_UI;
  out_data->out_flags2 =
      PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_SUPPORTS_QUERY_DYNAMIC_FLAGS;

  // Initialize globalData
  PF_Handle globalDataH = suites.HandleSuite1()->host_new_handle(sizeof(GlobalData));

  if (!globalDataH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  out_data->global_data = globalDataH;

  GlobalData* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(globalDataH));

  AEFX_CLR_STRUCT(*globalData);

  // Register with AEGP
  if (in_data->appl_id != 'PrMr') {
    ERR(suites.UtilitySuite3()->AEGP_RegisterWithAEGP(NULL, CONFIG_NAME, &globalData->aegpId));
  }

  // Initialize global OpenGL context
  globalData->context = new OGL::GlobalContext();
  if (!globalData->context->initialized) {
    return PF_Err_OUT_OF_MEMORY;
  }

  FX_LOG("OpenGL Version:       " << glGetString(GL_VERSION));
  FX_LOG("OpenGL Vendor:        " << glGetString(GL_VENDOR));
  FX_LOG("OpenGL Renderer:      " << glGetString(GL_RENDERER));
  FX_LOG("OpenGL GLSL Versions: " << glGetString(GL_SHADING_LANGUAGE_VERSION));

  globalData->context->bind();

  // Setup GL objects
  std::string resourcePath = AEUtil::getResourcesPath(in_data);

  globalData->defaultScene = VVISF::CreateISF4AESceneRef();
  globalData->defaultScene->useFile(resourcePath + "shaders/Default ISF4AE Shader.fs");

  globalData->ae2glScene = VVISF::CreateISF4AESceneRef();
  globalData->ae2glScene->useFile(resourcePath + "shaders/ae2gl.fs");

  globalData->gl2aeScene = VVISF::CreateISF4AESceneRef();
  globalData->gl2aeScene->useFile(resourcePath + "shaders/gl2ae.fs");

  globalData->scenes = new std::unordered_map<std::string, SceneDesc*>();

  auto defaultSceneDesc = new SceneDesc();
  defaultSceneDesc->status = "Not Loaded";
  defaultSceneDesc->scene = globalData->defaultScene;

  (*globalData->scenes)[""] = defaultSceneDesc;

  suites.HandleSuite1()->host_unlock_handle(globalDataH);

  return err;
}

/**
 * Free all global data (only required if you allocated some).
 */
static PF_Err GlobalSetdown(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  if (in_data->global_data) {
    // Dispose globalData
    // TODO: Find a way not to call destructors explicitly
    auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));

    globalData->context->bind();

    globalData->defaultScene.reset();
    globalData->gl2aeScene.reset();
    delete globalData->scenes;

    delete globalData->context;

    suites.HandleSuite1()->host_unlock_handle(in_data->global_data);
    suites.HandleSuite1()->host_dispose_handle(in_data->global_data);
  }
  return err;
}

static PF_Err SequenceSetup(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_Handle seqH = suites.HandleSuite1()->host_new_handle(sizeof(SequenceData));

  if (!seqH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  auto* seq = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(seqH));

  seq->showISFOption = true;

  out_data->sequence_data = seqH;

  suites.HandleSuite1()->host_unlock_handle(seqH);

  return err;
}

static PF_Err SequenceFlatten(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto unflatSeq = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(in_data->sequence_data));

  PF_Handle flatSeqH = suites.HandleSuite1()->host_new_handle(sizeof(SequenceData));

  if (!flatSeqH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  auto flatSeq = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(flatSeqH));

  flatSeq->showISFOption = unflatSeq->showISFOption;

  suites.HandleSuite1()->host_unlock_handle(flatSeqH);
  suites.HandleSuite1()->host_unlock_handle(in_data->sequence_data);

  out_data->sequence_data = flatSeqH;

  return err;
}

static PF_Err SequenceRestart(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto flatSeq = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(in_data->sequence_data));

  PF_Handle unflatSeqH = suites.HandleSuite1()->host_new_handle(sizeof(SequenceData));

  if (!unflatSeqH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  auto unflatSeq = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(unflatSeqH));

  unflatSeq->showISFOption = flatSeq->showISFOption;

  suites.HandleSuite1()->host_unlock_handle(unflatSeqH);
  suites.HandleSuite1()->host_unlock_handle(in_data->sequence_data);

  out_data->sequence_data = unflatSeqH;

  return err;
}

static PF_Err SequenceSetdown(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  suites.HandleSuite1()->host_dispose_handle(in_data->sequence_data);

  return err;
}

/**
 * Describe your parameters and register them using PF_ADD_PARAM.
 */
static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  // Customize the name of the options button
  // Premiere Pro/Elements does not support this suite
  if (in_data->appl_id != 'PrMr') {
    auto effectUISuite =
        AEFX_SuiteScoper<PF_EffectUISuite1>(in_data, kPFEffectUISuite, kPFEffectUISuiteVersion1, out_data);

    ERR(effectUISuite->PF_SetOptionsButtonName(in_data->effect_ref, "ISF Option.."));
  }

  PF_ParamDef def;

  // Add parameters
  AEFX_CLR_STRUCT(def);
  PF_ADD_TOPIC("ISF Option", Param_ISFGroupStart);

  // A bidden arbitrary param for storing fragment strings
  AEFX_CLR_STRUCT(def);
  def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_SUPERVISE;
  ERR(CreateDefaultArb(in_data, out_data, &def.u.arb_d.dephault));
  PF_ADD_ARBITRARY2("ISF", 1, 1,  // width, height
                    0, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE, def.u.arb_d.dephault, Param_ISF, ARB_REFCON);

  // "Edit Shader" button (also shows a shader compliation status)
  AEFX_CLR_STRUCT(def);
  PF_ADD_BUTTON("",
                "Load Shader",           // BUTTON_NAME
                PF_PUI_NONE,             // PUI_FLAGS
                PF_ParamFlag_SUPERVISE,  // PARAM_FLAGS
                Param_Edit);             // ID

  // "Save Shader" button
  AEFX_CLR_STRUCT(def);
  PF_ADD_BUTTON("",
                "Save Shader",           // BUTTON_NAME
                PF_PUI_NONE,             // PUI_FLAGS
                PF_ParamFlag_SUPERVISE,  // PARAM_FLAGS
                Param_Save);             // ID

  PF_END_TOPIC(Param_ISFGroupEnd);

  AEFX_CLR_STRUCT(def);
  PF_ADD_CHECKBOX("Use Layer Time",  // Label
                  "",                // A description right to the checkbox
                  FALSE,             // Default
                  PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY, Param_UseLayerTime);

  AEFX_CLR_STRUCT(def);
  PF_ADD_FLOAT_SLIDERX("Time", -1000000, 1000000,     // Valid range
                       0, 10,                         // Slider range
                       0,                             // Default
                       1,                             // Precision
                       PF_ValueDisplayFlag_NONE,      // Display
                       PF_ParamFlag_COLLAPSE_TWIRLY,  // Flags
                       Param_Time);                   // ID

  // Add all possible user params
  A_char name[32];

  for (int userParamIndex = 0; userParamIndex < NumUserParams; userParamIndex++) {
    PF_SPRINTF(name, "Bool %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX(name, "", FALSE, PF_ParamFlag_COLLAPSE_TWIRLY,
                    getIndexForUserParam(userParamIndex, UserParamType_Bool));

    PF_SPRINTF(name, "Long %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    def.flags |= PF_ParamFlag_COLLAPSE_TWIRLY;
    PF_ADD_POPUP("", 1, 0, "Choice", getIndexForUserParam(userParamIndex, UserParamType_Long));

    PF_SPRINTF(name, "Float %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(name, -1000000, 1000000,  // Valid range
                         0, 1,                     // Slider range
                         0,                        // Default
                         1,                        // Precision
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_COLLAPSE_TWIRLY,
                         getIndexForUserParam(userParamIndex, UserParamType_Float));

    PF_SPRINTF(name, "Angle %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    def.u.ad.valid_min = -1000000;
    def.u.ad.valid_max = +1000000;
    def.flags |= PF_ParamFlag_COLLAPSE_TWIRLY;
    PF_ADD_ANGLE(name, 0, getIndexForUserParam(userParamIndex, UserParamType_Angle));

    PF_SPRINTF(name, "Point2D %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    def.flags |= PF_ParamFlag_COLLAPSE_TWIRLY;
    PF_ADD_POINT(name,
                 // at a center of layer, with specifying in percentage
                 50L, 50L,
                 // restrict_bounds
                 0, getIndexForUserParam(userParamIndex, UserParamType_Point2D));

    PF_SPRINTF(name, "Color %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    def.flags |= PF_ParamFlag_COLLAPSE_TWIRLY;
    PF_ADD_COLOR(name, 1, 1, 1, getIndexForUserParam(userParamIndex, UserParamType_Color));

    PF_SPRINTF(name, "Image %d", userParamIndex);
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER(name, PF_LayerDefault_NONE, getIndexForUserParam(userParamIndex, UserParamType_Image));
  }

  if (!err) {
    // Referencing Examples/UI/CCU
    PF_CustomUIInfo ci;

    AEFX_CLR_STRUCT(ci);

    ci.events = PF_CustomEFlag_LAYER | PF_CustomEFlag_COMP;
    ci.comp_ui_width = ci.comp_ui_height = 0;
    ci.layer_ui_width = ci.layer_ui_height = 0;
    ci.preview_ui_width = ci.preview_ui_height = 0;
    ci.comp_ui_alignment = PF_UIAlignment_NONE;
    ci.layer_ui_alignment = PF_UIAlignment_NONE;
    ci.preview_ui_alignment = PF_UIAlignment_NONE;

    err = (*(in_data->inter.register_ui))(in_data->effect_ref, &ci);
  }

  // Set PF_OutData->num_params to match the parameter count.
  out_data->num_params = NumParams;

  return err;
}

static PF_Err SmartPreRender(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  // Create paramInfo
  PF_Handle paramInfoH = suites.HandleSuite1()->host_new_handle(sizeof(ParamInfo));

  if (!paramInfoH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  // Set handler
  extra->output->pre_render_data = paramInfoH;

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* paramInfo = reinterpret_cast<ParamInfo*>(suites.HandleSuite1()->host_lock_handle(paramInfoH));

  if (!paramInfo) {
    return PF_Err_OUT_OF_MEMORY;
  }

  PF_ParamDef paramDef;

  // Get the ISF scene from cache and save its pointer to ParamInfo
  {
    AEFX_CLR_STRUCT(paramDef);
    ERR(PF_CHECKOUT_PARAM(in_data, Param_ISF, in_data->current_time, in_data->time_step, in_data->time_scale,
                          &paramDef));

    auto* isf = reinterpret_cast<ParamArbIsf*>(*paramDef.u.arb_d.value);

    if (isf) {
      auto desc = getCompiledSceneDesc(globalData, isf->code);

      paramInfo->scene = desc->scene.get();
    }

    ERR2(PF_CHECKIN_PARAM(in_data, &paramDef));
  }

  // Checkout all image parameters
  int userParamIndex = 0;
  PF_CheckoutResult inResult;

  for (auto input : paramInfo->scene->inputs()) {
    UserParamType userParamType = getUserParamTypeForISFAttr(input);

    if (userParamType == UserParamType_Image) {
      bool isInputImage = input->name() == "inputImage";

      PF_ParamIndex paramIndex = isInputImage ? Param_Input : getIndexForUserParam(userParamIndex, UserParamType_Image);

      PF_RenderRequest req = extra->input->output_request;

      req.rect.left = -100000;
      req.rect.top = -100000;
      req.rect.right = 100000;
      req.rect.bottom = 100000;
      req.preserve_rgb_of_zero_alpha = true;

      ERR(extra->cb->checkout_layer(in_data->effect_ref,
                                    // A parameter index of layer to checkout
                                    paramIndex,
                                    // Unique index for retriving pixels in Cmd_SmartRender
                                    // -- just use paramIndex itself.
                                    paramIndex, &req, in_data->current_time, in_data->time_step, in_data->time_scale,
                                    &inResult));

      VVGL::Size& size = paramInfo->inputImageSizes[userParamIndex];
      size.width = inResult.ref_width * in_data->downsample_x.num / in_data->downsample_x.den;
      size.height = inResult.ref_height * in_data->downsample_y.num / in_data->downsample_y.den;
    }

    if (isISFAttrVisibleInECW(input)) {
      userParamIndex++;
    }
  }

  // Compute the rect to render
  if (!err) {
    // Set the output region to an entire layer multiplied by downsample, regardless of input's mask.
    PF_Rect outputRect = {
        0, 0, (A_long)std::ceil((double)in_data->width * in_data->downsample_x.num / in_data->downsample_x.den),
        (A_long)std::ceil((double)in_data->height * in_data->downsample_y.num / in_data->downsample_y.den)};

    UnionLRect(&outputRect, &extra->output->result_rect);
    UnionLRect(&outputRect, &extra->output->max_result_rect);

    paramInfo->outSize = VVGL::Size(outputRect.right, outputRect.bottom);

    extra->output->flags |= PF_RenderOutputFlag_RETURNS_EXTRA_PIXELS;
  }

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);
  suites.HandleSuite1()->host_unlock_handle(paramInfoH);

  return err;
}

static PF_Err SmartRender(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_PointParamSuite1* pointSuite;
  ERR(AEFX_AcquireSuite(in_data, out_data, kPFPointParamSuite, kPFPointParamSuiteVersion1, "Couldn't load suite.",
                        (void**)&pointSuite));

  auto paramInfoH = reinterpret_cast<PF_Handle>(extra->input->pre_render_data);

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* paramInfo = reinterpret_cast<ParamInfo*>(suites.HandleSuite1()->host_lock_handle(paramInfoH));

  // OpenGL
  if (!err) {
    globalData->context->bind();

    auto bitdepth = extra->input->bitdepth;
    auto pixelBytes = bitdepth * 4 / 8;
    VVGL::Size& outSize = paramInfo->outSize;

    // In After Effects, 16-bit pixel doesn't use the highest bit, and thus each channel ranges 0x0000 - 0x8000.
    // So after passing pixel buffer to GPU, it should be scaled by (0xffff / 0x8000) to normalize the luminance to
    // 0.0-1.0.
    VVISF::ISFVal multiplier16bit(VVISF::ISFValType_Float, bitdepth == 16 ? (65535.0f / 32768.0f) : 1.0f);
    globalData->ae2glScene->setValueForInputNamed(multiplier16bit, "multiplier16bit");
    globalData->gl2aeScene->setValueForInputNamed(multiplier16bit, "multiplier16bit");

    // Render ISF
    auto isfImage = createRGBATexWithBitdepth(outSize, bitdepth);
    {
      auto& scene = *paramInfo->scene;

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
        auto isfType = input->type();
        auto& name = input->name();
        auto userParamType = getUserParamTypeForISFAttr(input);
        bool isInputImage = name == "inputImage";
        auto paramIndex = isInputImage ? Param_Input : getIndexForUserParam(userParamIndex, userParamType);

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
            if (name == "i4a_Downsample") {
              point.x = (float)in_data->downsample_x.num / in_data->downsample_x.den;
              point.y = (float)in_data->downsample_y.num / in_data->downsample_y.den;
            } else {
              ERR(AEUtil::getPointParam(in_data, out_data, paramIndex, &point));
              // Should be converted to normalized and vertically-flipped coordinate
              point.x = point.x / outSize.width;
              point.y = 1.0 - point.y / outSize.height;
            }
            val = new VVISF::ISFVal(isfType, point.x, point.y);
            break;
          }
          case UserParamType_Color: {
            PF_PixelFloat color;
            ERR(AEUtil::getColorParam(in_data, out_data, paramIndex, &color));
            val = new VVISF::ISFVal(isfType, color.red, color.green, color.blue, color.alpha);
            break;
          }
          case UserParamType_Image: {
            PF_LayerDef* imagePixels = nullptr;
            ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, paramIndex, &imagePixels)));

            // imagePixels will be staying null when the parameter has set to empty.
            if (imagePixels != nullptr) {
              VVGL::Size imageSize(imagePixels->width, imagePixels->height);
              VVGL::Size bufferSizeInPixel(imagePixels->rowbytes / pixelBytes, imageSize.height);
              VVGL::Size& outImageSize = paramInfo->inputImageSizes[userParamIndex];

              VVGL::GLBufferRef inputImageAECPU =
                  createRGBACPUBufferWithBitdepthUsing(bufferSizeInPixel, imagePixels->data, imageSize, bitdepth);

              auto inputImageAE = globalData->context->uploader->uploadCPUToTex(inputImageAECPU);

              ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, paramIndex));

              // Note that AE's inputImage is cropped by mask's region and smaller than ISF resolution.
              glBindTexture(GL_TEXTURE_2D, inputImageAE->name);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
              glBindTexture(GL_TEXTURE_2D, 0);

              auto origin = VVISF::ISFVal(VVISF::ISFValType_Point2D, imagePixels->origin_x, imagePixels->origin_y);

              globalData->ae2glScene->setBufferForInputNamed(inputImageAE, "inputImage");
              globalData->ae2glScene->setValueForInputNamed(origin, "origin");

              VVGL::GLBufferRef inputImage = createRGBATexWithBitdepth(outImageSize, bitdepth);

              globalData->ae2glScene->renderToBuffer(inputImage);

              scene.setBufferForInputNamed(inputImage, name);
            }

            break;
          }

          default:
            FX_LOG("Invalid ISFValType.");
            break;
        }

        if (val != nullptr) {
          scene.setValueForInputNamed(*val, input->name());
        }

        if (isISFAttrVisibleInECW(input)) {
          userParamIndex++;
        }
      }  // End of for each ISF->inputs

      // Then, render it!
      scene.renderToBuffer(isfImage, outSize, time);
    }

    // Download the result of ISF
    PF_EffectWorld* output_worldP = nullptr;
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

    if (output_worldP != nullptr && output_worldP->width <= in_data->width &&
        output_worldP->height <= in_data->height) {
      auto& scene = *globalData->gl2aeScene;

      scene.setBufferForInputNamed(isfImage, "inputImage");

      auto outputImage = createRGBATexWithBitdepth(outSize, bitdepth);
      scene.VVGL::GLScene::renderToBuffer(outputImage);
      auto outputImageCPU = globalData->context->downloader->downloadTexToCPU(outputImage);

      char* glP = nullptr;  // OpenGL
      char* aeP = nullptr;  // AE

      auto bytesPerRowGl = outputImageCPU->calculateBackingBytesPerRow();

      // Copy per row
      for (size_t y = 0; y < outSize.height; y++) {
        glP = (char*)outputImageCPU->cpuBackingPtr + y * bytesPerRowGl;
        aeP = (char*)output_worldP->data + y * output_worldP->rowbytes;
        std::memcpy(aeP, glP, outSize.width * pixelBytes);
      }
    }

    // VVGL: Release any old buffer
    VVGL::GetGlobalBufferPool()->housekeeping();

  }  // End OpenGL

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);
  suites.HandleSuite1()->host_unlock_handle(paramInfoH);
  suites.HandleSuite1()->host_dispose_handle(paramInfoH);

  ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite, kPFWorldSuiteVersion2, "Couldn't release suite."));

  return err;
}

static PF_Err UserChangedParam(PF_InData* in_data,
                               PF_OutData* out_data,
                               PF_ParamDef* params[],
                               const PF_UserChangedParamExtra* which_hit) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));

  switch (which_hit->param_index) {
    case Param_Edit: {
      // Load a shader
      std::vector<std::string> fileTypes = {"fs", "txt", "frag", "glsl"};

      std::string srcPath = SystemUtil::openFileDialog(fileTypes);

      if (!srcPath.empty()) {
        std::string isfCode = SystemUtil::readTextFile(srcPath);

        if (!isfCode.empty()) {
          params[Param_ISF]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;

          auto* isf = reinterpret_cast<ParamArbIsf*>(*params[Param_ISF]->u.arb_d.value);
          isf->name = getBasename(srcPath);
          isf->code = isfCode;

          ERR(AEUtil::setEffectName(globalData->aegpId, in_data, isf->name));

          // Set default values
          auto* desc = getCompiledSceneDesc(globalData, isf->code);

          int userParamIndex = 0;

          for (auto& input : desc->scene->inputs()) {
            if (!isISFAttrVisibleInECW(input)) {
              continue;
            }

            auto userParamType = getUserParamTypeForISFAttr(input);
            auto index = getIndexForUserParam(userParamIndex, userParamType);
            auto& param = *params[index];

            param.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;

            switch (userParamType) {
              case UserParamType_Bool:
                param.u.bd.dephault = input->defaultVal().getBoolVal();
                break;

              case UserParamType_Long: {
                auto values = input->valArray();
                auto dephaultVal = input->defaultVal().getLongVal();
                A_long dephaultIndex = mmax(1, findIndex(values, dephaultVal));

                param.u.pd.value = dephaultIndex;
                break;
              }

              case UserParamType_Float: {
                double dephault = input->defaultVal().getDoubleVal();

                if (input->unit() == VVISF::ISFValUnit_Length) {
                  dephault *= in_data->width;
                } else if (input->unit() == VVISF::ISFValUnit_Percent) {
                  dephault *= 100;
                }

                param.u.fs_d.value = dephault;
                break;
              }

              case UserParamType_Angle: {
                double rad = input->defaultVal().getDoubleVal();
                param.u.ad.value = FLOAT2FIX(-(rad * 180.0 / PI) + 90.0);
                break;
              }

              case UserParamType_Point2D: {
                auto x = input->defaultVal().getPointValByIndex(0);
                auto y = input->defaultVal().getPointValByIndex(1);

                param.u.td.x_value = FLOAT2FIX(x * in_data->width);
                param.u.td.y_value = FLOAT2FIX((1.0 - y) * in_data->height);
                break;
              }

              case UserParamType_Color: {
                auto dephault = input->defaultVal();

                param.u.cd.value.red = dephault.getColorValByChannel(0) * 255;
                param.u.cd.value.green = dephault.getColorValByChannel(1) * 255;
                param.u.cd.value.blue = dephault.getColorValByChannel(2) * 255;
                param.u.cd.value.alpha = dephault.getColorValByChannel(3) * 255;
                break;
              }

              default:
                break;
            }

            userParamIndex++;
          }  // End of for-each ISF's inputs

        } else {
          // On failed reading the text file, or simply it's empty
          suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "Cannot open the shader file: %s",
                                                srcPath.c_str());
          out_data->out_flags = PF_OutFlag_DISPLAY_ERROR_MESSAGE;
        }
      }
    } break;
    case Param_Save:
      saveISF(in_data, out_data);
      break;
  }

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return PF_Err_NONE;
}

static PF_Err UpdateParamsUI(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
  PF_Err err = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* seqData = reinterpret_cast<SequenceData*>(suites.HandleSuite1()->host_lock_handle(in_data->sequence_data));

  auto* isf = reinterpret_cast<ParamArbIsf*>(*params[Param_ISF]->u.arb_d.value);
  auto* desc = getCompiledSceneDesc(globalData, isf->code);

  // Toggle the visibility of ISF options
  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_ISFGroupStart, seqData->showISFOption));
  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_ISFGroupEnd, seqData->showISFOption));

  // Set the shader status as a label for 'Edit Shader'
  std::string statusLabel = "ISF: " + desc->status;
  AEUtil::setParamName(globalData->aegpId, in_data, params, Param_ISFGroupStart, statusLabel);

  // Show the time parameters if the current shader is time dependant
  bool isTimeDependant = desc->scene->isTimeDependant();
  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_UseLayerTime, isTimeDependant));

  // Toggle the visibility of 'Time' parameter depending on 'Use Layer Time'
  A_Boolean useLayerTime = params[Param_UseLayerTime]->u.bd.value;
  ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, Param_Time, !useLayerTime && isTimeDependant));

  // Change the visiblity of user params
  PF_ParamIndex userParamIndex = 0;
  UserParamType userParamType;

  auto inputs = desc->scene->inputs();

  for (auto& input : inputs) {
    if (userParamIndex >= inputs.size()) {
      break;
    }

    if (!isISFAttrVisibleInECW(input)) {
      continue;
    }

    userParamType = getUserParamTypeForISFAttr(input);

    auto index = getIndexForUserParam(userParamIndex, userParamType);

    // Update the value and other UI options
    auto& param = *params[index];

    switch (userParamType) {
      case UserParamType_Bool: {
        param.u.bd.dephault = input->defaultVal().getBoolVal();
        break;
      }

      case UserParamType_Long: {
        auto labels = input->labelArray();
        auto values = input->valArray();

        param.u.pd.num_choices = labels.size();

        auto joinedLabels = joinWith(labels, "|");

        A_char* names = new A_char[joinedLabels.length() + 1];
        PF_STRCPY(names, joinedLabels.c_str());

        param.u.pd.u.namesptr = names;

        auto dephaultVal = input->defaultVal().getLongVal();
        A_long dephaultIndex = mmax(1, findIndex(values, dephaultVal));

        param.u.pd.dephault = dephaultIndex;

        break;
      }

      case UserParamType_Float: {
        auto dephault = input->defaultVal().getDoubleVal();
        auto min = input->minVal().getDoubleVal();
        auto max = input->maxVal().getDoubleVal();
        PF_ValueDisplayFlags displayFlags = PF_ValueDisplayFlag_NONE;

        if (input->unit() == VVISF::ISFValUnit_Length) {
          dephault = 0;
          min = 0;
          max = 100;
        } else if (input->unit() == VVISF::ISFValUnit_Percent) {
          dephault *= 100;
          min *= 100;
          max *= 100;
          displayFlags |= PF_ValueDisplayFlag_PERCENT;
        }

        param.u.fs_d.dephault = dephault;
        param.u.fs_d.slider_min = min;
        param.u.fs_d.slider_max = max;
        param.u.fs_d.display_flags = displayFlags;

        break;
      }

      case UserParamType_Angle: {
        double deg = input->defaultVal().getDoubleVal();
        param.u.ad.dephault = FLOAT2FIX(-(deg * 180.0 / PI) + 90.0);
        ;

        break;
      }

      case UserParamType_Point2D: {
        auto x = input->defaultVal().getPointValByIndex(0);
        auto y = input->defaultVal().getPointValByIndex(1);

        param.u.td.x_dephault = FLOAT2FIX(x * in_data->width);
        param.u.td.y_dephault = FLOAT2FIX((1.0 - y) * in_data->height);
        break;
      }

      case UserParamType_Color: {
        auto dephault = input->defaultVal();

        param.u.cd.dephault.red = dephault.getColorValByChannel(0) * 255;
        param.u.cd.dephault.green = dephault.getColorValByChannel(1) * 255;
        param.u.cd.dephault.blue = dephault.getColorValByChannel(2) * 255;
        param.u.cd.dephault.alpha = dephault.getColorValByChannel(3) * 255;

        break;
      }

      default:
        break;
    }

    param.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;

    // Set the label and visibility
    for (int type = 0; type < NumUserParamType; type++) {
      auto index = getIndexForUserParam(userParamIndex, (UserParamType)type);
      bool visible = type == userParamType;

      ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, index, visible));

      if (visible) {
        // Set label
        auto label = input->label();
        if (label.empty())
          label = input->name();

        ERR(AEUtil::setParamName(globalData->aegpId, in_data, params, index, label));
      }
    }

    userParamIndex++;
  }

  // Hide all out-of-range parameters
  for (; userParamIndex < NumUserParams; userParamIndex++) {
    for (int userParamType = 0; userParamType < NumUserParamType; userParamType++) {
      auto index = getIndexForUserParam(userParamIndex, (UserParamType)userParamType);
      ERR(AEUtil::setParamVisibility(globalData->aegpId, in_data, params, index, false));
    }
  }

  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);
  suites.HandleSuite1()->host_unlock_handle(in_data->sequence_data);

  return err;
}

static PF_Err QueryDynamicFlags(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], void* extra) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef def;

  AEFX_CLR_STRUCT(def);

  //    The parameter array passed with PF_Cmd_QUERY_DYNAMIC_FLAGS
  //    contains invalid values; use PF_CHECKOUT_PARAM() to obtain
  //    valid values.

  ERR(PF_CHECKOUT_PARAM(in_data, Param_UseLayerTime, in_data->current_time, in_data->time_step, in_data->time_scale,
                        &def));

  if (!err) {
    auto useLayerTime = def.u.bd.value;
    setBitFlag(PF_OutFlag_NON_PARAM_VARY, useLayerTime, &out_data->out_flags);
  }

  ERR2(PF_CHECKIN_PARAM(in_data, &def));

  return err;
}

extern "C" DllExport PF_Err PluginDataEntryFunction(PF_PluginDataPtr inPtr,
                                                    PF_PluginDataCB inPluginDataCallBackPtr,
                                                    SPBasicSuite* inSPBasicSuitePtr,
                                                    const char* inHostName,
                                                    const char* inHostVersion) {
  PF_Err result = PF_Err_INVALID_CALLBACK;

  result = PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, CONFIG_NAME, CONFIG_MATCH_NAME, CONFIG_CATEGORY,
                              AE_RESERVED_INFO);  // Reserved Info

  return result;
}

PF_Err EffectMain(PF_Cmd cmd,
                  PF_InData* in_data,
                  PF_OutData* out_data,
                  PF_ParamDef* params[],
                  PF_LayerDef* output,
                  void* extra) {
  PF_Err err = PF_Err_NONE;

  try {
    switch (cmd) {
      case PF_Cmd_ABOUT:
        err = About(in_data, out_data, params, output);
        break;

      case PF_Cmd_DO_DIALOG:
        err = PopDialog(in_data, out_data, params, output);
        break;

      case PF_Cmd_GLOBAL_SETUP:
        err = GlobalSetup(in_data, out_data, params, output);
        break;

      case PF_Cmd_PARAMS_SETUP:
        err = ParamsSetup(in_data, out_data, params, output);
        break;

      case PF_Cmd_GLOBAL_SETDOWN:
        err = GlobalSetdown(in_data, out_data, params, output);
        break;

      case PF_Cmd_ARBITRARY_CALLBACK:
        err = HandleArbitrary(in_data, out_data, params, output, reinterpret_cast<PF_ArbParamsExtra*>(extra));
        break;

      case PF_Cmd_SEQUENCE_SETUP:
        err = SequenceSetup(in_data, out_data, params, output);
        break;

      case PF_Cmd_SEQUENCE_FLATTEN:
        err = SequenceFlatten(in_data, out_data, params, output);
        break;

      case PF_Cmd_SEQUENCE_RESETUP:
        err = SequenceRestart(in_data, out_data, params, output);
        break;

      case PF_Cmd_SEQUENCE_SETDOWN:
        err = SequenceSetdown(in_data, out_data, params, output);
        break;

      case PF_Cmd_SMART_PRE_RENDER:
        err = SmartPreRender(in_data, out_data, reinterpret_cast<PF_PreRenderExtra*>(extra));
        break;

      case PF_Cmd_SMART_RENDER:
        err = SmartRender(in_data, out_data, reinterpret_cast<PF_SmartRenderExtra*>(extra));
        break;

      case PF_Cmd_USER_CHANGED_PARAM:
        err = UserChangedParam(in_data, out_data, params, reinterpret_cast<const PF_UserChangedParamExtra*>(extra));
        break;

      case PF_Cmd_UPDATE_PARAMS_UI:
        // To change the label for 'Edit Shader' button
        err = UpdateParamsUI(in_data, out_data, params, output);
        break;

      case PF_Cmd_QUERY_DYNAMIC_FLAGS:
        err = QueryDynamicFlags(in_data, out_data, params, extra);
        break;

      case PF_Cmd_EVENT:
        err = HandleEvent(in_data, out_data, params, output, reinterpret_cast<PF_EventExtra*>(extra));
        break;
    }
  } catch (PF_Err& thrown_err) {
    err = thrown_err;
  }
  return err;
}

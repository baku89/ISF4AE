#include "GLSLCanvas.h"

#include "Smart_Utils.h"
#include "AEFX_SuiteHelper.h"

#include "SystemUtil.h"
#include "Config.h"
#include "AEUtils.hpp"

#include "AEOGLInterop.hpp"

#include <iostream>

/**
 * Display a dialog describing the plug-in. Populate out_data>return_msg and After Effects will display it in a simple modal dialog.
 */
static PF_Err
About(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output) {
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
                                          "%s v%d.%d\r%s",
                                          CONFIG_NAME,
                                          MAJOR_VERSION,
                                          MINOR_VERSION,
                                          CONFIG_DESCRIPTION);
    return PF_Err_NONE;
}

/**
 * Set any required flags and PF_OutData fields (including out_data>my_version) to describe your plug-in’s behavior.
 */
static PF_Err
GlobalSetup(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    out_data->my_version = PF_VERSION(MAJOR_VERSION,
                                      MINOR_VERSION,
                                      BUG_VERSION,
                                      STAGE_VERSION,
                                      BUILD_VERSION);

    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
    out_data->out_flags2 = PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_SMART_RENDER;
    
    // Initialize globalData
    auto handleSuite = suites.HandleSuite1();
    PF_Handle globalDataH = handleSuite->host_new_handle(sizeof(GlobalData));
    
    if (!globalDataH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    out_data->global_data = globalDataH;
    
    GlobalData *globalData = reinterpret_cast<GlobalData *>(
        handleSuite->host_lock_handle(globalDataH));

    // Initialize global OpenGL context
    globalData->context = *new OGL::GlobalContext();
    if (!globalData->context.initialized) {
        return PF_Err_OUT_OF_MEMORY;
    }

    globalData->context.bind();

    // Setup GL objects
    globalData->fbo = *new OGL::Fbo();
    globalData->quad = *new OGL::QuadVao();
    
    std::string resourcePath = AEUtils::getResourcesPath(in_data);
    std::string vertPath = resourcePath + "shaders/passthru.vert";
    std::string fragPath = resourcePath + "shaders/uv-gradient.frag";
    globalData->shader = *new OGL::Shader();
    
    globalData->shader.initialize(vertPath.c_str(), fragPath.c_str());
    
    handleSuite->host_unlock_handle(globalDataH);
    
    FX_LOG("OpenGL Version:       " << glGetString(GL_VERSION));
    FX_LOG("OpenGL Vendor:        " << glGetString(GL_VENDOR));
    FX_LOG("OpenGL Renderer:      " << glGetString(GL_RENDERER));
    FX_LOG("OpenGL GLSL Versions: " << glGetString(GL_SHADING_LANGUAGE_VERSION));

    return err;
}

/**
 * Free all global data (only required if you allocated some).
 */
static PF_Err
GlobalSetdown(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output) {
    
PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    // Dispose globalData
    auto globalDataH = suites.HandleSuite1()->host_lock_handle(in_data->global_data);
    auto *globalData = reinterpret_cast<GlobalData *>(globalDataH);

    globalData->fbo.~Fbo();
    globalData->quad.~QuadVao();
    globalData->context.~GlobalContext();

    suites.HandleSuite1()->host_dispose_handle(in_data->global_data);

    return err;
}

/**
 * Describe your parameters and register them using PF_ADD_PARAM.
 */
static PF_Err
ParamsSetup(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output) {
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    PF_ParamDef def;
    
    // Add parameters
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Time",
                         -1000000,                  // VALID_MIN
                         1000000,                   // VALID_MAX
                         0,                         // SLIDER_MIN
                         10,                        // SLIDER_MAX
                         0,                         // Default
                         6,                         // Precision
                         PF_ValueDisplayFlag_NONE,  // Display
                         0,                         // Flags
                         PARAM_TIME);               // ID

    // TODO: set default mouse position to center of layer
    AEFX_CLR_STRUCT(def);
    PF_ADD_POINT("Mouse Position", 0, 0, false, PARAM_MOUSE);

    AEFX_CLR_STRUCT(def);

    // Set PF_OutData->num_params to match the parameter count.
    out_data->num_params = NUM_PARAMS;

    return err;
}

static PF_Err PreRender(PF_InData *in_data, PF_OutData *out_data,
                        PF_PreRenderExtra *extra) {
    PF_Err err = PF_Err_NONE;


    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult in_result;

    // Create paramInfo
    AEFX_SuiteScoper<PF_HandleSuite1> handleSuite
        = AEFX_SuiteScoper<PF_HandleSuite1>(in_data,
                                            kPFHandleSuite,
                                            kPFHandleSuiteVersion1,
                                            out_data);

    PF_Handle paramInfoH = handleSuite->host_new_handle(sizeof(ParamInfo));
    
    if (!paramInfoH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    // Set handler
    extra->output->pre_render_data = paramInfoH;
    
    ParamInfo *paramInfo = reinterpret_cast<ParamInfo*>(handleSuite->host_lock_handle(paramInfoH));
    
    if (!paramInfo) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    // Checkout Params
    PF_ParamDef param_time;
    AEFX_CLR_STRUCT(param_time);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_TIME,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &param_time));
    
    PF_ParamDef param_mouse;
    AEFX_CLR_STRUCT(param_mouse);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_MOUSE,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &param_mouse));
    
    // Assign latest param values
    ERR(AEOGLInterop::getFloatSliderParam(in_data,
                                          out_data,
                                          PARAM_TIME,
                                          &paramInfo->time));

    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_MOUSE,
                                    AEOGLInterop::GL_SPACE, &paramInfo->mouse));

    handleSuite->host_unlock_handle(paramInfoH);

    // Checkout input image
    ERR(extra->cb->checkout_layer(in_data->effect_ref, PARAM_INPUT, PARAM_INPUT,
                                  &req, in_data->current_time,
                                  in_data->time_step, in_data->time_scale,
                                  &in_result));
    
    // Compute the rect to render
    if (!err) {
        UnionLRect(&in_result.result_rect, &extra->output->result_rect);
        UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
    }
    
    return err;
}

static PF_Err SmartRender(PF_InData *in_data, PF_OutData *out_data,
                          PF_SmartRenderExtra *extra) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    AEGP_SuiteHandler suites(in_data->pica_basicP);

    PF_EffectWorld *input_worldP = nullptr, *output_worldP = nullptr;
    PF_WorldSuite2 *wsP = nullptr;

    // Retrieve paramInfo
    auto handleSuite = suites.HandleSuite1();

    ParamInfo *paramInfo =
        reinterpret_cast<ParamInfo *>(handleSuite->host_lock_handle(
            reinterpret_cast<PF_Handle>(extra->input->pre_render_data)));

    // Checkout layer pixels
    ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, PARAM_INPUT,
                                          &input_worldP)));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

    // Setup wsP
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFWorldSuite,
                          kPFWorldSuiteVersion2, "Couldn't load suite.",
                          (void **)&wsP));

    // Get pixel format
    PF_PixelFormat format = PF_PixelFormat_INVALID;
    ERR(wsP->PF_GetPixelFormat(input_worldP, &format));

    auto *globalData = reinterpret_cast<GlobalData *>(
        handleSuite->host_lock_handle(in_data->global_data));

    // OpenGL
    if (!err) {
        globalData->context.bind();

        GLenum pixelType;
        switch (format) {
            case PF_PixelFormat_ARGB32:
                pixelType = GL_UNSIGNED_BYTE;
                break;
            case PF_PixelFormat_ARGB64:
                pixelType = GL_UNSIGNED_SHORT;
                break;
            case PF_PixelFormat_ARGB128:
                pixelType = GL_FLOAT;
                break;
        }

        GLsizei width = input_worldP->width;
        GLsizei height = input_worldP->height;
        size_t pixelBytes = AEOGLInterop::getPixelBytes(pixelType);

        // Setup render context
        globalData->fbo.allocate(width, height, GL_RGBA, pixelType);

        // Allocate pixels buffer
        PF_Handle pixelsBufferH =
            handleSuite->host_new_handle(width * height * pixelBytes);
        void *pixelsBufferP = reinterpret_cast<char *>(
            handleSuite->host_lock_handle(pixelsBufferH));
        
        // Bind
        globalData->shader.bind();
        globalData->fbo.bind();

        // Set uniforms
        globalData->shader.setVec2("u_resolution", width, height);
        globalData->shader.setFloat("u_time", paramInfo->time);
        globalData->shader.setVec2("u_mouse", paramInfo->mouse.x, paramInfo->mouse.y);
        
        // Render
        globalData->quad.render();

        // Read pixels
        globalData->fbo.readToPixels(pixelsBufferP);
        ERR(AEOGLInterop::downloadTexture(pixelsBufferP, output_worldP, pixelType));

        // Unbind
        globalData->shader.unbind();
        globalData->fbo.unbind();

        // downloadTexture
        ERR(AEOGLInterop::downloadTexture(pixelsBufferP, output_worldP, pixelType));

        handleSuite->host_unlock_handle(pixelsBufferH);
        handleSuite->host_dispose_handle(pixelsBufferH);
    }

    // Check in
    ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite,
                           kPFWorldSuiteVersion2, "Couldn't release suite."));
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));

    return err;
}

extern "C" DllExport PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr inPtr, PF_PluginDataCB inPluginDataCallBackPtr,
    SPBasicSuite *inSPBasicSuitePtr, const char *inHostName,
    const char *inHostVersion) {
    PF_Err result = PF_Err_INVALID_CALLBACK;

    result =
        PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, CONFIG_NAME,
                           CONFIG_MATCH_NAME, CONFIG_CATEGORY,
                           AE_RESERVED_INFO);  // Reserved Info
    
    return result;
}

DllExport
    PF_Err EffectMain(
        PF_Cmd cmd,
        PF_InData *in_data,
        PF_OutData *out_data,
        PF_ParamDef *params[],
        PF_LayerDef *output,
        void *extra) {
        
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data,
                            out_data,
                            params,
                            output);
                break;

            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data,
                                  out_data,
                                  params,
                                  output);
                break;
                
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data,
                                  out_data,
                                  params,
                                  output);
                break;

            case PF_Cmd_GLOBAL_SETDOWN:
                err = GlobalSetdown(in_data,
                                    out_data,
                                    params,
                                    output);
                break;

            case PF_Cmd_SMART_PRE_RENDER:
                err = PreRender(in_data, out_data,
                                reinterpret_cast<PF_PreRenderExtra *>(extra));
                break;
            
            case PF_Cmd_SMART_RENDER:
                err = SmartRender(in_data, out_data,
                                  reinterpret_cast<PF_SmartRenderExtra *>(extra));
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}

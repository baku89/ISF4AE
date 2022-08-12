#include "GLSLCanvas.h"

#include "Smart_Utils.h"
#include "AEFX_SuiteHelper.h"

#include "Config.h"

#include "SystemUtil.h"
#include "AEUtil.h"

#include "AEOGLInterop.hpp"

#include <iostream>

/**
 * Compile a shader and store to pool that maps code string to OGL::Program.
 * It will be called at UpdateParameterUI and PreRender.
 */
void compileShaderIfNeeded(GlobalData *globalData, A_char *code) {
    // Compile a shader at here to make sure to display the latest status
    auto &programRefs = *globalData->programRefs;

    if (strlen(code) > 0 && programRefs.find(code) == programRefs.end()) {
        // Compile new shader if not exists
        globalData->context->bind();
        OGL::Shader frag(code, GL_FRAGMENT_SHADER);

        if (!frag.isSucceed()) {
            // On failed compiling a shader
            ProgramRef *pr = new ProgramRef();
            pr->error = PROGRAM_ERROR_SHADER;
            pr->infoLog = frag.getInfoLog();
            programRefs[code] = pr;

        } else {
            OGL::Program *prog = new OGL::Program(globalData->passthruVertShader, &frag);

            if (!prog->isSucceed()) {
                // On falied linking
                ProgramRef *pr = new ProgramRef();
                pr->error = PROGRAM_ERROR_LINK;
                pr->infoLog = prog->getInfoLog();
                programRefs[code] = pr;

                delete prog;
            } else {
                // On succeed
                ProgramRef *pr = new ProgramRef();
                pr->error = PROGRAM_NO_ERROR;
                pr->program = prog;
                programRefs[code] = pr;
            }
        }
    }
}

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
 * Pops up shader errors on user clicked Option label of the effect
 */
static PF_Err
PopDialog (
   PF_InData        *in_data,
   PF_OutData        *out_data,
   PF_ParamDef        *params[],
   PF_LayerDef        *output )
{
    PF_Err err = PF_Err_NONE;
    
    auto *globalData = reinterpret_cast<GlobalData*>(DH(out_data->global_data));
    auto *glsl = reinterpret_cast<ParamArbGlsl*>(*params[PARAM_GLSL]->u.arb_d.value);
    
    auto &programRefs = *globalData->programRefs;
    auto &code = glsl->fragCode;
    
    A_char *status;
    std::string infoLog = "";
    
    if (strlen(code) == 0) {
        status = "Not Loaded";
    } else if (programRefs.find(code) == programRefs.end()) {
        status = "Not Compiled";
    } else {
        auto pr = programRefs[code];
        
        switch (pr->error) {
            case PROGRAM_NO_ERROR:
                status = "Compiled Successfully";
                break;
                
            case PROGRAM_ERROR_SHADER:
                status = "Shader Error";
                infoLog = pr->infoLog;
                break;
                
            case PROGRAM_ERROR_LINK:
                status = "Link Error";
                infoLog = pr->infoLog;
                break;
        }
    }
    
    const char *separator = infoLog.length() > 0 ? "\n---\n" : "";
    
    PF_SPRINTF(out_data->return_msg, "Shader Status: %s%s%s", status, separator, infoLog.c_str());
    out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    
    return err;
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

    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_CUSTOM_UI | PF_OutFlag_I_DO_DIALOG;
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
    globalData->fboIntermediate = new OGL::Fbo();
    globalData->fboFinal = new OGL::Fbo();
    globalData->quad = new OGL::QuadVao();
    globalData->inputTexture = new OGL::Texture();
    
    std::string resourcePath = AEUtil::getResourcesPath(in_data);
    std::string passthruVert = SystemUtil::readTextFile(resourcePath + "shaders/passthru.vert");
    std::string defaultFrag = SystemUtil::readTextFile(resourcePath + "shaders/default.frag");
    std::string swizzleFrag = SystemUtil::readTextFile(resourcePath + "shaders/swizzle.frag");
    
    globalData->passthruVertShader = new OGL::Shader(passthruVert.c_str(), GL_VERTEX_SHADER);
    
    OGL::Shader defaultFragShader(defaultFrag.c_str(), GL_FRAGMENT_SHADER);
    OGL::Shader swizzleFragShader(swizzleFrag.c_str(), GL_FRAGMENT_SHADER);
    
    globalData->defaultProgram = new OGL::Program(globalData->passthruVertShader, &defaultFragShader);
    globalData->swizzleProgram = new OGL::Program(globalData->passthruVertShader, &swizzleFragShader);
    
    auto programRefs = new std::unordered_map<std::string,  ProgramRef*>();
    
    globalData->programRefs = programRefs;
    
    handleSuite->host_unlock_handle(globalDataH);

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
    
    if (in_data->global_data) {
    
        // Dispose globalData
        // TODO: Find a way not to call destructors explicitly
        auto globalDataH = suites.HandleSuite1()->host_lock_handle(in_data->global_data);
        auto *globalData = reinterpret_cast<GlobalData *>(globalDataH);
            
        globalData->context->bind();

        delete globalData->fboIntermediate;
        delete globalData->fboFinal;
        delete globalData->quad;
        delete globalData->inputTexture;
        delete globalData->passthruVertShader;
        delete globalData->defaultProgram;
        delete globalData->swizzleProgram;
        delete globalData->programRefs;
        
        delete globalData->context;
        
        suites.HandleSuite1()->host_dispose_handle(in_data->global_data);
    }
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
    
    // Customize the name of the options button
    // Premiere Pro/Elements does not support this suite
    if (in_data->appl_id != 'PrMr') {
        AEFX_SuiteScoper<PF_EffectUISuite1> effectUISuite = AEFX_SuiteScoper<PF_EffectUISuite1>(in_data,
                                                                                                kPFEffectUISuite,
                                                                                                kPFEffectUISuiteVersion1,
                                                                                                out_data);
        
        ERR(effectUISuite->PF_SetOptionsButtonName(in_data->effect_ref, "Show Error.."));
    }

    PF_ParamDef def;
    
    // Add parameters
        
    // A bidden arbitrary param for storing fragment strings
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_SUPERVISE;
    ERR(CreateDefaultArb(in_data, out_data, &def.u.arb_d.dephault));
    PF_ADD_ARBITRARY2("GLSL",
                      1, 1, // width, height
                      0,
                      PF_PUI_NO_ECW_UI,
                      def.u.arb_d.dephault,
                      PARAM_GLSL,
                      ARB_REFCON);
    
    // "Edit Shader" button (also shows a shader compliation status)
    AEFX_CLR_STRUCT(def);
    PF_ADD_BUTTON("No Shader Loaded",
                  "Load Shader",                  // BUTTON_NAME
                  PF_PUI_NONE,                    // PUI_FLAGS
                  PF_ParamFlag_SUPERVISE,         // PARAM_FLAGS
                  PARAM_EDIT);                    // ID
   
    // "Edit Shader" button
    AEFX_CLR_STRUCT(def);
    PF_ADD_BUTTON("",
                  "Save Shader",                  // BUTTON_NAME
                  PF_PUI_NONE,                    // PUI_FLAGS
                  PF_ParamFlag_SUPERVISE,         // PARAM_FLAGS
                  PARAM_SAVE);                    // ID
    
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

static PF_Err
HandleArbitrary(
    PF_InData            *in_data,
    PF_OutData            *out_data,
    PF_ParamDef            *params[],
    PF_LayerDef            *output,
    PF_ArbParamsExtra    *extra)
{
    PF_Err     err     = PF_Err_NONE;
    
    switch (extra->which_function) {
        case PF_Arbitrary_NEW_FUNC:
            if (extra->u.new_func_params.refconPV != ARB_REFCON) {
                err = PF_Err_INTERNAL_STRUCT_DAMAGED;
            } else {
                err = CreateDefaultArb(in_data, out_data, extra->u.new_func_params.arbPH);
            }
            break;
            
        case PF_Arbitrary_DISPOSE_FUNC:
            if (extra->u.dispose_func_params.refconPV != ARB_REFCON) {
                err = PF_Err_INTERNAL_STRUCT_DAMAGED;
            } else {
                PF_DISPOSE_HANDLE(extra->u.dispose_func_params.arbH);
            }
            break;
        
        case PF_Arbitrary_COPY_FUNC:
            if(extra->u.copy_func_params.refconPV == ARB_REFCON) {
                ERR(CreateDefaultArb(in_data,
                                     out_data,
                                     extra->u.copy_func_params.dst_arbPH));

                ERR(ArbCopy(in_data,
                            out_data,
                            &extra->u.copy_func_params.src_arbH,
                            extra->u.copy_func_params.dst_arbPH));
            }
            break;
        
        case PF_Arbitrary_FLAT_SIZE_FUNC:
            *(extra->u.flat_size_func_params.flat_data_sizePLu) = sizeof(ParamArbGlsl);
            break;

        case PF_Arbitrary_FLATTEN_FUNC:
            if(extra->u.flatten_func_params.buf_sizeLu == sizeof(ParamArbGlsl)){
                void *src = (ParamArbGlsl*)PF_LOCK_HANDLE(extra->u.flatten_func_params.arbH);
                void *dst = extra->u.flatten_func_params.flat_dataPV;
                if (src) {
                    memcpy(dst, src, sizeof(ParamArbGlsl));
                }
                PF_UNLOCK_HANDLE(extra->u.flatten_func_params.arbH);
            }
            break;

        case PF_Arbitrary_UNFLATTEN_FUNC:
            if(extra->u.unflatten_func_params.buf_sizeLu == sizeof(ParamArbGlsl)){
                PF_Handle    handle = PF_NEW_HANDLE(sizeof(ParamArbGlsl));
                void *dst = (ParamArbGlsl*)PF_LOCK_HANDLE(handle);
                void *src = (void*)extra->u.unflatten_func_params.flat_dataPV;
                if (src) {
                    memcpy(dst, src, sizeof(ParamArbGlsl));
                }
                *(extra->u.unflatten_func_params.arbPH) = handle;
                PF_UNLOCK_HANDLE(handle);
            }
            break;
        
        case PF_Arbitrary_COMPARE_FUNC:
            ERR(ArbCompare(in_data,
                           out_data,
                           &extra->u.compare_func_params.a_arbH,
                           &extra->u.compare_func_params.b_arbH,
                           extra->u.compare_func_params.compareP));
            break;
    }
    
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
    
    // Get the GLSL prgoram from cache
    auto *globalData = reinterpret_cast<GlobalData *>(
        handleSuite->host_lock_handle(in_data->global_data));
    
    PF_ParamDef paramGlsl;
    AEFX_CLR_STRUCT(paramGlsl);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_GLSL,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &paramGlsl));
    
    ParamArbGlsl *glsl = reinterpret_cast<ParamArbGlsl*>(*paramGlsl.u.arb_d.value);
    
    if (glsl) {
        auto &code = glsl->fragCode;
        auto &programRefs = *globalData->programRefs;
        
        compileShaderIfNeeded(globalData, code);        
        
        paramInfo->program = globalData->defaultProgram;
        
        if (strlen(code) > 0 && programRefs.find(code) != programRefs.end()) {
            ProgramRef *pr = programRefs[code];
            if (pr->error == PROGRAM_NO_ERROR) {
                paramInfo->program = pr->program;
            }
        }
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
        globalData->context->bind();

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

        // Setup texture and fbos
        globalData->fboFinal->allocate(width, height, GL_RGBA, pixelType);
        globalData->fboIntermediate->allocate(width, height, GL_RGBA, pixelType);
        globalData->inputTexture->allocate(width, height, GL_RGBA, pixelType);

        // Allocate pixels buffer
        PF_Handle pixelsBufferH =
            handleSuite->host_new_handle(width * height * pixelBytes);
        void *pixelsBufferP = reinterpret_cast<char *>(
            handleSuite->host_lock_handle(pixelsBufferH));
        
        {
            // Pass 1: Render glslCanvas-style code to fboIntermediate
            OGL::Program &program = *paramInfo->program;
            program.bind();
            globalData->fboIntermediate->bind();

            program.setVec2("u_resolution", width, height);
            program.setFloat("u_time", paramInfo->time);
            program.setVec2("u_mouse", paramInfo->mouse.x, paramInfo->mouse.y);
            
            globalData->quad->render();
        }
        
        {
            // Pass 2: Swizzle RGBA to ARGB and apply a mask
            OGL::Program &program = *globalData->swizzleProgram;
            program.bind();
            globalData->fboFinal->bind();
            
            AEOGLInterop::uploadTexture(globalData->inputTexture,
                                        input_worldP, pixelsBufferP, pixelType);
            
            float multiplier16bit = AEOGLInterop::getMultiplier16bit(pixelType);
            
            program.setFloat("multiplier16bit", multiplier16bit);
            program.setVec2("u_resolution", width, height);
            program.setTexture("inputTexture", globalData->inputTexture, 0);
            program.setTexture("glslCanvasOutputTexture", globalData->fboIntermediate->getTexture(), 1);
            
            globalData->quad->render();
        }

        // Read pixels
        globalData->fboFinal->readToPixels(pixelsBufferP);
        ERR(AEOGLInterop::downloadTexture(pixelsBufferP, output_worldP, pixelType));
        
        // Release the pixel buffer
        handleSuite->host_unlock_handle(pixelsBufferH);
        handleSuite->host_dispose_handle(pixelsBufferH);
    }

    // Check in
    ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite,
                           kPFWorldSuiteVersion2, "Couldn't release suite."));
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));

    return err;
}

static PF_Err
UserChangedParam(PF_InData *in_data,
                 PF_OutData *out_data,
                 PF_ParamDef *params[],
                 const PF_UserChangedParamExtra *which_hit)
{
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    switch (which_hit->param_index) {
        case PARAM_EDIT:
        {
            // Load a shader
            std::vector<std::string> fileTypes;
                
            fileTypes.push_back("frag");
            fileTypes.push_back("glsl");
            fileTypes.push_back("fs");
        
            std::string srcPath = SystemUtil::openFileDialog(fileTypes);
            
            if (!srcPath.empty()) {
                std::string fragCode = SystemUtil::readTextFile(srcPath);
                
                if (!fragCode.empty()) {
                    params[PARAM_GLSL]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
                    
                    ParamArbGlsl *arb = reinterpret_cast<ParamArbGlsl*>(*params[PARAM_GLSL]->u.arb_d.value);
                    PF_STRCPY(arb->fragCode, fragCode.c_str());
                    
                    ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref,
                                                                    PARAM_GLSL,
                                                                    params[PARAM_GLSL]));
                } else {
                    // On failed reading the text file, or simply it's empty
                    suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
                                                          "Cannot open the shader file: %s", srcPath.c_str());
                    out_data->out_flags = PF_OutFlag_DISPLAY_ERROR_MESSAGE;
                }
            }
        }
            break;
        case PARAM_SAVE:
            // Save the current shader
            auto *globalData = reinterpret_cast<GlobalData *>(
                suites.HandleSuite1()->host_lock_handle(in_data->global_data));
            
            // Set name of an effect instance as default file name
            // https://ae-plugins.docsforadobe.dev/aegps/aegp-suites.html#streamrefs-and-effectrefs
            AEGP_EffectRefH effectH = NULL;
            AEGP_StreamRefH glslStreamH, effectStreamH;
            A_char effectName[AEGP_MAX_ITEM_NAME_SIZE] = "shader.frag";
            
            ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(globalData->aegpId,
                                                                       in_data->effect_ref,
                                                                       &effectH));

            ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(globalData->aegpId,
                                                                      effectH,
                                                                      PARAM_GLSL,
                                                                      &glslStreamH));
            
            ERR(suites.DynamicStreamSuite4()->AEGP_GetNewParentStreamRef(globalData->aegpId,
                                                                         glslStreamH,
                                                                         &effectStreamH));
            
            ERR(suites.StreamSuite2()->AEGP_GetStreamName(effectStreamH,
                                                          FALSE,
                                                          effectName));
            
            
            // Then confirm a destination path and save it
            std::string dstPath = SystemUtil::saveFileDialog(std::string(effectName) + ".frag");
            
            if (!err && !dstPath.empty()) {
                PF_ParamDef param_glsl;
                AEFX_CLR_STRUCT(param_glsl);
                ERR(PF_CHECKOUT_PARAM(in_data,
                                      PARAM_GLSL,
                                      in_data->current_time,
                                      in_data->time_step,
                                      in_data->time_scale,
                                      &param_glsl));
                
                ParamArbGlsl *arb = reinterpret_cast<ParamArbGlsl*>(*param_glsl.u.arb_d.value);
                SystemUtil::writeTextFile(dstPath, arb->fragCode);
                
                ERR(PF_CHECKIN_PARAM(in_data, &param_glsl));
            }
            break;
    }
    
    return PF_Err_NONE;
    
}

static PF_Err
UpdateParameterUI(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    // Create copies of all parameters
    PF_ParamDef newParams[NUM_PARAMS];
    for (A_short i = 0; i < NUM_PARAMS; i++) {
        AEFX_CLR_STRUCT(newParams[i]);
        newParams[i] = *params[i];
    }
    
    auto *globalData = reinterpret_cast<GlobalData *>(
        suites.HandleSuite1()->host_lock_handle(in_data->global_data));
    
    PF_ParamDef paramGlsl;
    AEFX_CLR_STRUCT(paramGlsl);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_GLSL,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &paramGlsl));
    
    ParamArbGlsl *glsl = reinterpret_cast<ParamArbGlsl*>(*paramGlsl.u.arb_d.value);
    
    if (!glsl) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    
    auto &code = glsl->fragCode;
    
    // Compile a shader at here to make sure to display the latest compilation status
    compileShaderIfNeeded(globalData, code);
    
    // Set the shader compliation status
    A_char *shaderStatus = "Not Loaded";
    
    if (strlen(code) > 0) {
        auto &programRefs = *globalData->programRefs;
        
        if (programRefs.find(code) == programRefs.end()) {
            shaderStatus = "Not Compiled";
        } else {
            switch (programRefs[code]->error) {
                case PROGRAM_NO_ERROR:
                    shaderStatus = "Compiled Successfully";
                    break;
                case PROGRAM_ERROR_SHADER:
                    shaderStatus = "Shader Error";
                    break;
                case PROGRAM_ERROR_LINK:
                    shaderStatus = "Link Error";
                    break;
            }
        }
    }
    
    PF_STRCPY(newParams[PARAM_EDIT].name, shaderStatus);
    
    ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref,
                                                    PARAM_EDIT,
                                                    &newParams[PARAM_EDIT]));
    
    out_data->out_flags |= PF_OutFlag_REFRESH_UI;
    
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
            
            case PF_Cmd_DO_DIALOG:
                err = PopDialog(in_data,
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
            
            case PF_Cmd_ARBITRARY_CALLBACK:
                err = HandleArbitrary(in_data,
                                      out_data,
                                      params,
                                      output,
                                      reinterpret_cast<PF_ArbParamsExtra*>(extra));
                break;

            case PF_Cmd_SMART_PRE_RENDER:
                err = PreRender(in_data, out_data,
                                reinterpret_cast<PF_PreRenderExtra *>(extra));
                break;
            
            case PF_Cmd_SMART_RENDER:
                err = SmartRender(in_data, out_data,
                                  reinterpret_cast<PF_SmartRenderExtra *>(extra));
                break;
                
            case PF_Cmd_USER_CHANGED_PARAM:
                err = UserChangedParam(in_data,
                                       out_data,
                                       params,
                                       reinterpret_cast<const PF_UserChangedParamExtra*>(extra));
                break;
            
            case PF_Cmd_UPDATE_PARAMS_UI:
                // To change the label for 'Edit Shader' button
                err = UpdateParameterUI(in_data,
                                        out_data,
                                        params,
                                        output);
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}

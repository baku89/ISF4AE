#include "AEUtil.h"

#include "AEConfig.h"
#include "AE_EffectCB.h"

namespace AEUtil {

std::string getResourcesPath(PF_InData *in_data) {
   // initialize and compile the shader objects
   A_UTF16Char pluginFolderPath[AEFX_MAX_PATH];
   PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);

#ifdef AE_OS_WIN
   std::string resourcePath = get_string_from_wcs((wchar_t *)pluginFolderPath);
   std::string::size_type pos;
   // delete the plugin name
   pos = resourcePath.rfind("\\", resourcePath.length());
   resourcePath = resourcePath.substr(0, pos) + "\\";
#endif
#ifdef AE_OS_MAC
   NSUInteger length = 0;
   A_UTF16Char *tmp = pluginFolderPath;
   while (*tmp++ != 0) {
       ++length;
   }
   NSString *newStr =
       [[NSString alloc] initWithCharacters:pluginFolderPath
                                     length:length];
   std::string resourcePath([newStr UTF8String]);
   resourcePath += "/Contents/Resources/";
#endif
   return resourcePath;
}


PF_Err setParamVisibility(AEGP_PluginID aegpId,
                          PF_InData *in_data,
                          PF_ParamDef *params[],
                          PF_ParamIndex index,
                          A_Boolean visible) {
    
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    A_Boolean invisible = !visible;
    
    // For Premiere Pro
    PF_ParamDef newParam;
    AEFX_CLR_STRUCT(newParam);
    newParam = *params[index];
    
    setBitFlag(PF_PUI_INVISIBLE, invisible, &newParam.ui_flags);
    
    ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref,
                                                    index,
                                                    &newParam));
    
    // For After Effects
    AEGP_EffectRefH effectH = nullptr;
    AEGP_StreamRefH streamH = nullptr;
    
    ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(aegpId,
                                                               in_data->effect_ref,
                                                               &effectH));
    

    ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(aegpId,
                                                              effectH,
                                                              index,
                                                              &streamH));

    ERR(suites.DynamicStreamSuite4()->AEGP_SetDynamicStreamFlag(streamH,
                                                                AEGP_DynStreamFlag_HIDDEN,
                                                                FALSE,
                                                                invisible));
    
    if (effectH) ERR(suites.EffectSuite4()->AEGP_DisposeEffect(effectH));
    if (streamH) ERR(suites.StreamSuite5()->AEGP_DisposeStream(streamH));
    
    return err;
}

PF_Err setParamName(PF_InData *in_data,
                    PF_ParamDef *params[],
                    PF_ParamIndex index,
                    std::string &name) {
    
    PF_Err err = PF_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    PF_ParamDef newParam;
    newParam = *params[index];
    
    PF_STRCPY(newParam.name, name.c_str());
    
    ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref,
                                                    index,
                                                    &newParam));
    
    return err;
}

/**
 * Retrives a coordinate in pixel from point-type parameter with considering downsample
 */
PF_Err getPointParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_FloatPoint *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));

    PF_PointParamSuite1 *pointSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFPointParamSuite,
                          kPFPointParamSuiteVersion1, "Couldn't load suite.",
                          (void **)&pointSuite));

    // value takes downsampled coord
    ERR(pointSuite->PF_GetFloatingPointValueFromPointDef(in_data->effect_ref,
                                                         &param_def, value));

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getAngleParam(PF_InData *in_data, PF_OutData *out_data, int paramId,
                     A_FpLong *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));

    PF_AngleParamSuite1 *angleSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFAngleParamSuite,
                          kPFAngleParamSuiteVersion1, "Couldn't load suite.",
                          (void **)&angleSuite));

    ERR(angleSuite->PF_GetFloatingPointValueFromAngleDef(in_data->effect_ref,
                                                         &param_def, value));

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));
    return err;
}

PF_Err getPopupParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_long *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.pd.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getFloatSliderParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_FpLong *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.fs_d.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getCheckboxParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_Boolean *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.bd.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

/**
 Retrieves color parameter, in between 0.0-1.0 float value for each channel
 */
PF_Err getColorParam(PF_InData *in_data, PF_OutData *out_data, int paramIndex, PF_PixelFloat *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    PF_ParamDef paramDef;
    AEFX_CLR_STRUCT(paramDef);
    ERR(PF_CHECKOUT_PARAM(in_data, paramIndex, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &paramDef));
    
    ERR(suites.ColorParamSuite1()->PF_GetFloatingPointColorFromColorDef(in_data->effect_ref,
                                                                        &paramDef,
                                                                        value));
    
    ERR2(PF_CHECKIN_PARAM(in_data, &paramDef));

    return err;
}


}

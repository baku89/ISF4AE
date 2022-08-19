#pragma once

#include "OGL.h"

// make sure we get 16bpc pixels;
// AE_Effect.h checks for this.
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "String_Utils.h"

#include <memory>

#define PI 3.14159265358979323864f

namespace AEOGLInterop {

/**
  * In After Effects, 16-bit pixel doesn't use the highest bit, and thus each channel ranges 0x0000 - 0x8000. So after passing pixel buffer to GPU, it should be scaled by (0xffff / 0x8000) to normalize the luminance to 0.0-1.0.
 */
float getMultiplier16bit(short bitdepth) {
    return bitdepth == 16 ? (65535.0f / 32768.0f) : 1.0f;
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

}  // namespace AEOGLInterop

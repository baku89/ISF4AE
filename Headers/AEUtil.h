#pragma once

#include "AE_Effect.h"

#include <string>

#include "AE_Macros.h"
#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"

#include "MiscUtil.h"

namespace AEUtil {

std::string getResourcesPath(PF_InData *in_data);

PF_Err setParamVisibility(AEGP_PluginID aegpId,
                          PF_InData *in_data,
                          PF_ParamDef *params[],
                          PF_ParamIndex index,
                          A_Boolean visible);

PF_Err setParamName(PF_InData *in_data,
                    PF_ParamDef *params[],
                    PF_ParamIndex index,
                    std::string &name);

// Getters for parameters
PF_Err getPointParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_FloatPoint *value);
PF_Err getAngleParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_FpLong *value);
PF_Err getPopupParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_long *value);
PF_Err getFloatSliderParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_FpLong *value);
PF_Err getCheckboxParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_Boolean *value);
PF_Err getColorParam(PF_InData *in_data, PF_OutData *out_data, int paramIndex, PF_PixelFloat *value);

}  // namespace AEUtil

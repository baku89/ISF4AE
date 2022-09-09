#pragma once

#include "AE_Effect.h"

#include <string>

#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"
#include "AE_Macros.h"

#include "MiscUtil.h"

namespace AEUtil {

std::string getResourcesPath(PF_InData* in_data);

PF_Err setParamVisibility(AEGP_PluginID aegpId,
                          PF_InData* in_data,
                          PF_ParamDef* params[],
                          PF_ParamIndex index,
                          A_Boolean visible);

PF_Err setParamName(AEGP_PluginID aegpId,
                    PF_InData* in_data,
                    PF_ParamDef* params[],
                    PF_ParamIndex index,
                    std::string& name);

// Getters for parameters
PF_Err getPointParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_FloatPoint* value);
PF_Err getAngleParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_FpLong* value);
PF_Err getPopupParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_long* value);
PF_Err getFloatSliderParam(PF_InData* in_data, PF_OutData* out_data, int paramId, PF_FpLong* value);
PF_Err getCheckboxParam(PF_InData* in_data, PF_OutData* out_data, int paramId, PF_Boolean* value);
PF_Err getColorParam(PF_InData* in_data, PF_OutData* out_data, int paramIndex, PF_PixelFloat* value);

// AEGP utils
PF_Err getEffectName(AEGP_PluginID aegpId, PF_InData* in_data, std::string* name);
PF_Err setEffectName(AEGP_PluginID aegpId, PF_InData* in_data, const std::string& name);

PF_Err getStringPersistentData(PF_InData* in_data,
                               const A_char* sectionKey,
                               const A_char* valueKey,
                               const std::string& defaultValue,
                               std::string* value);
PF_Err setStringPersistentData(PF_InData* in_data,
                               const A_char* sectionKey,
                               const A_char* valueKey,
                               const std::string& value);

// Other AE-specific utils
void copyConvertStringLiteralIntoUTF16(const wchar_t* inputString, A_UTF16Char* destination);

}  // namespace AEUtil

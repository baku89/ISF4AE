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


}  // namespace AEUtil

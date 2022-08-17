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

}

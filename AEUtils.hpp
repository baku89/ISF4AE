#pragma once

#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include <string>

namespace AEUtils {
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

}  // namespace AEUtils

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

}

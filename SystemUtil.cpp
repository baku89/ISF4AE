#include "SystemUtil.h"


namespace AESDK_SystemUtil {
	
	std::string openFileDialog(std::vector<std::string> &fileTypes) {
		
		std::string path;
		
		NSArray *nsFileTypes = @[@"frag", @"glsl", @"fs", @"txt"];
		NSOpenPanel * panel = [NSOpenPanel openPanel];
		[panel setAllowsMultipleSelection: NO];
		[panel setCanChooseDirectories: NO];
		[panel setCanChooseFiles: YES];
		[panel setFloatingPanel: YES];
		[panel setAllowedFileTypes: nsFileTypes];
		NSInteger result = [panel runModal];
							
		if(result == NSModalResponseOK) {
			NSString *nsPath = [panel URLs][0].absoluteString;
			nsPath = [nsPath stringByReplacingOccurrencesOfString:@"file://" withString:@""];
			path = std::string([nsPath UTF8String]);
		}
		
		return path;
	}
	
	/*
	std::string GetResourcesPath(PF_InData *in_data) {
		//initialize and compile the shader objects
		A_UTF16Char pluginFolderPath[AEFX_MAX_PATH];
		PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);
		
		NSUInteger length = 0;
		A_UTF16Char* tmp = pluginFolderPath;
		while (*tmp++ != 0) {
			++length;
		}
		NSString* newStr = [[NSString alloc] initWithCharacters:pluginFolderPath length : length];
		std::string resourcePath([newStr UTF8String]);
		resourcePath += "/Contents/Resources/";
		
		return resourcePath;
	}*/
	
	
};

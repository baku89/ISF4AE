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
	
};

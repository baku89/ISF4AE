#pragma once

#ifndef SystemUtil_h
#define SystemUtil_h

#include <string>
#include <vector>


namespace AESDK_SystemUtil {

std::string openFileDialog(std::vector<std::string> &fileTypes);

std::string readTextFile(std::string path);
	
};



#endif /* SystemUtil_h */

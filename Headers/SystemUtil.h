#pragma once

#include <string>
#include <vector>


namespace SystemUtil {

std::string openFileDialog(std::vector<std::string> &fileTypes);

std::string readTextFile(std::string path);
	
};

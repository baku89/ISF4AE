#pragma once

#include <string>
#include <vector>


namespace SystemUtil {

std::string openFileDialog(std::vector<std::string> &fileTypes);
std::string saveFileDialog(const std::string &filename);

std::string readTextFile(std::string path);
bool writeTextFile(std::string path, std::string text);
	
};

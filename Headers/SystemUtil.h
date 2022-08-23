#pragma once

#include <string>
#include <vector>

namespace SystemUtil {

std::string openFileDialog(const std::vector<std::string>& fileTypes);
std::string saveFileDialog(const std::string& filename);

std::string readTextFile(const std::string& path);
bool writeTextFile(const std::string& path, const std::string& text);

};  // namespace SystemUtil

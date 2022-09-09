#pragma once

#include <string>
#include <vector>

namespace SystemUtil {

std::string openFileDialog(const std::vector<std::string>& fileTypes,
                           const std::string& directory = "",
                           const std::string& title = "");
std::string saveFileDialog(const std::string& filename,
                           const std::string& directory = "",
                           const std::string& title = "");

std::string readTextFile(const std::string& path);
bool writeTextFile(const std::string& path, const std::string& text);

};  // namespace SystemUtil

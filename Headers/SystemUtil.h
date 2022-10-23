#pragma once

#include <string>
#include <vector>

using namespace std;

namespace SystemUtil {

string openFileDialog(const vector<string>& fileTypes, const string& directory = "", const string& title = "");
string saveFileDialog(const string& filename, const string& directory = "", const string& title = "");

string readTextFile(const string& path);
bool writeTextFile(const string& path, const string& text);

};  // namespace SystemUtil

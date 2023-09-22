#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <string>
#include <commdlg.h> // Loading and save file dialog
#include "../Win/resource.h"
#endif

using namespace std;

namespace SystemUtil {

string openFileDialog(const vector<string>& fileTypes, const string& directory = "", const string& title = "");
string saveFileDialog(const string& filename, const string& directory = "", const string& title = "");

string readTextFile(const string& path);
#ifdef _WIN32
string readResourceShader(UINT resourceID);
#else
string readResourceShader(const string& path);
#endif

bool writeTextFile(const string& path, const string& text);

};  // namespace SystemUtil

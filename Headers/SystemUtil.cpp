#include "SystemUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Debug.h"

namespace SystemUtil {

#ifdef _WIN32
string openFileDialog(const vector<string>& fileTypes, const string& directory, const string& title) {
  OPENFILENAME ofn;
  char szFile[260] = {0};

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrTitle = title.c_str();
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  // Convert fileTypes to filter pattern
  string filter;
  for (const auto& type : fileTypes) {
    filter += type + " files" + '\0' + "*." + type + '\0';
  }
  filter += '\0';
  ofn.lpstrFilter = filter.c_str();

  if (GetOpenFileName(&ofn) == TRUE) {
    return ofn.lpstrFile;
  }

  return "";
}
#else
string openFileDialog(const vector<string>& fileTypes, const string& directory, const string& title) {
  string path;

  id nsFileTypes = [NSMutableArray new];

  for (auto& fileType : fileTypes) {
    id nsStr = [NSString stringWithUTF8String:fileType.c_str()];
    [nsFileTypes addObject:nsStr];
  }

  NSOpenPanel* panel = [NSOpenPanel openPanel];

  [panel setMessage:[NSString stringWithUTF8String:title.c_str()]];
  [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:directory.c_str()]]];
  [panel setAllowsMultipleSelection:NO];
  [panel setCanChooseDirectories:NO];
  [panel setCanChooseFiles:YES];
  [panel setFloatingPanel:YES];
  [panel setAllowedFileTypes:nsFileTypes];
  NSInteger result = [panel runModal];

  if (result == NSModalResponseOK) {
    NSString* nsPath = [panel URLs][0].absoluteString;
    nsPath = [nsPath stringByRemovingPercentEncoding];
    nsPath = [nsPath stringByReplacingOccurrencesOfString:@"file://" withString:@""];
    path = string([nsPath UTF8String]);
  }

  return path;
}
#endif

#ifdef _WIN32
string saveFileDialog(const string& filename, const string& directory, const string& title) {
  OPENFILENAME ofn;
  char szFile[260] = {0};
  strcpy(szFile, filename.c_str());

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrTitle = title.c_str();
  ofn.Flags = OFN_OVERWRITEPROMPT;

  if (GetSaveFileName(&ofn) == TRUE) {
    return ofn.lpstrFile;
  }

  return "";
}
#else
string saveFileDialog(const string& filename, const string& directory, const string& title) {
  string path;

  NSString* nsFilename = [NSString stringWithCString:filename.c_str() encoding:NSUTF8StringEncoding];

  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setMessage:[NSString stringWithUTF8String:title.c_str()]];
  [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:directory.c_str()]]];
  [panel setCanCreateDirectories:YES];
  [panel setNameFieldStringValue:nsFilename];
  [panel setFloatingPanel:YES];
  NSInteger result = [panel runModal];

  if (result == NSModalResponseOK) {
    NSString* nsPath = [panel URL].absoluteString;
    nsPath = [nsPath stringByRemovingPercentEncoding];
    nsPath = [nsPath stringByReplacingOccurrencesOfString:@"file://" withString:@""];
    path = string([nsPath UTF8String]);
  }

  return path;
}
#endif

string readTextFile(const string& path) {
  string text;
  ifstream file;

  // ensure ifstream objects can throw exceptions:
  file.exceptions(ifstream::failbit | ifstream::badbit);

  try {
    stringstream stream;

    file.open(path);

    stream << file.rdbuf();

    file.close();

    text = stream.str();

  } catch (...) {
    FX_LOG("Couldn't read a text from the specified path: " << path);
  }

  return text;
}

#ifdef _WIN32
static HMODULE GCM() {
  HMODULE hModule = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GCM, &hModule);

  return hModule;
}
string readResourceShader(UINT resourceID) {
  try {
    const char* resourceType = "CUSTOM";
    HMODULE hModule = GCM();  // Modeule Descriptor

    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceID), resourceType);
    if (hResource == NULL) {
      throw std::runtime_error("Failed to find the resource.");
    }

    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    if (hLoadedResource == NULL) {
      throw std::runtime_error("Failed to load the resource.");
    }

    LPVOID pLockedResource = LockResource(hLoadedResource);
    if (pLockedResource == NULL) {
      throw std::runtime_error("Failed to lock the resource.");
    }

    DWORD dwResourceSize = SizeofResource(hModule, hResource);
    if (dwResourceSize == 0) {
      throw std::runtime_error("Invalid resource size.");
    }

    // Converting resources data to a string
    return std::string((char*)pLockedResource, dwResourceSize);
  } catch (...) {
    FX_LOG("Couldn't read a text from the resource with ID: " << resourceID);
    return "";
  }
}
#else
string readResourceShader(const string& path) {
  return readTextFile(path);
}
#endif  // _WIN32

bool writeTextFile(const string& path, const string& text) {
  ofstream file;

  // ensure ifstream objects can throw exceptions:
  file.exceptions(ifstream::failbit | ifstream::badbit);

  try {
    file.open(path);

    file << text;

    file.close();

  } catch (...) {
    FX_LOG("Couldn't write a text to the specified file: " << path);
    return false;
  }

  return true;
}

};  // namespace SystemUtil

#include "SystemUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Debug.h"

namespace SystemUtil {

std::string openFileDialog(const std::vector<std::string>& fileTypes) {
  std::string path;

  id nsFileTypes = [NSMutableArray new];

  for (auto& fileType : fileTypes) {
    id nsStr = [NSString stringWithUTF8String:fileType.c_str()];
    [nsFileTypes addObject:nsStr];
  }

  NSOpenPanel* panel = [NSOpenPanel openPanel];
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
    path = std::string([nsPath UTF8String]);
  }

  return path;
}

std::string saveFileDialog(const std::string& filename) {
  std::string path;

  NSString* nsFilename = [NSString stringWithCString:filename.c_str() encoding:[NSString defaultCStringEncoding]];

  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setCanCreateDirectories:YES];
  [panel setNameFieldStringValue:nsFilename];
  [panel setFloatingPanel:YES];
  NSInteger result = [panel runModal];

  if (result == NSModalResponseOK) {
    NSString* nsPath = [panel URL].absoluteString;
    nsPath = [nsPath stringByRemovingPercentEncoding];
    nsPath = [nsPath stringByReplacingOccurrencesOfString:@"file://" withString:@""];
    path = std::string([nsPath UTF8String]);
  }

  return path;
}

std::string readTextFile(const std::string& path) {
  std::string text;
  std::ifstream file;

  // ensure ifstream objects can throw exceptions:
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    std::stringstream stream;

    file.open(path);

    stream << file.rdbuf();

    file.close();

    text = stream.str();

  } catch (...) {
    FX_LOG("Couldn't read a text from the specified path: " << path);
  }

  return text;
}

bool writeTextFile(const std::string& path, const std::string& text) {
  std::ofstream file;

  // ensure ifstream objects can throw exceptions:
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

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

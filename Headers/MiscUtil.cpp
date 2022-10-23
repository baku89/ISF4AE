#include "MiscUtil.h"

#include <filesystem>

string joinWith(const vector<string>& texts, const string& delimiter) {
  stringstream ss;

  for (int i = 0; i < texts.size(); i++) {
    ss << texts[i];

    if (i != texts.size() - 1) {
      ss << delimiter;
    }
  }

  return ss.str();
}

vector<string> splitWith(string s, string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  string token;
  vector<string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));

  return res;
}

void setBitFlag(int flag, bool value, int* target) {
  if (value) {
    *target |= flag;
  } else {
    *target &= ~flag;
  }
}

string getBasename(const string& path) {
  filesystem::path p(path);
  return p.stem();
}

string getDirname(const string& path) {
  filesystem::path p(path);
  return p.parent_path().string();
}

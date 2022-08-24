#include "MiscUtil.h"

std::string joinWith(const std::vector<std::string>& texts, const std::string& delimiter) {
  std::stringstream ss;

  for (int i = 0; i < texts.size(); i++) {
    ss << texts[i];

    if (i != texts.size() - 1) {
      ss << delimiter;
    }
  }

  return ss.str();
}

std::vector<std::string> splitWith(std::string s, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
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

#pragma once

#include <sstream>
#include <string>
#include <vector>

std::string joinWith(const std::vector<std::string>& texts, const std::string& delimiter);

/**
 * TODO: Generic function must be implemented in a header file
 * https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
 */
template <class T>
int findIndex(std::vector<T> vals, T target) {
  // https://www-cns-s-u--tokyo-ac-jp.translate.goog/~masuoka/post/search_vector_index/?_x_tr_sl=ja&_x_tr_tl=en&_x_tr_hl=en&_x_tr_pto=sc
  auto itr = std::find(vals.begin(), vals.end(), target);

  if (itr == vals.end()) {
    return -1;
  } else {
    return std::distance(vals.begin(), itr);
  }
}

void setBitFlag(int flag, bool value, int* target);

#pragma once

#include <sstream>
#include <string>
#include <vector>

using namespace std;

string joinWith(const vector<string>& texts, const string& delimiter);

vector<string> splitWith(string s, string delimiter);

/**
 * TODO: Generic function must be implemented in a header file
 * https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
 */
template <class T>
int findIndex(vector<T> vals, T target) {
  // https://www-cns-s-u--tokyo-ac-jp.translate.goog/~masuoka/post/search_vector_index/?_x_tr_sl=ja&_x_tr_tl=en&_x_tr_hl=en&_x_tr_pto=sc
  auto itr = find(vals.begin(), vals.end(), target);

  if (itr == vals.end()) {
    return -1;
  } else {
    return (int)distance(vals.begin(), itr);
  }
}

void setBitFlag(int flag, bool value, int* target);

string getBasename(const string& path);

string getDirname(const string& path);

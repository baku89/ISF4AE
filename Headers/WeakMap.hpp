#pragma once

#include <memory>
#include <unordered_map>

using namespace std;

/**
 * Emulates EcmaScripts' WeakMap in C++
 */
template <class K, class V>
class WeakMap {
 private:
  unordered_map<K, weak_ptr<V>> _map;

 public:
  shared_ptr<V> get(const K& key);
  bool has(const K& key);
  void set(const K& key, const shared_ptr<V>& value);
};

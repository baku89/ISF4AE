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
  shared_ptr<V> get(const K& key) {
    if (!has(key)) {
      return nullptr;
    }
    return _map[key].lock();
  };

  bool has(const K& key) {
    if (_map.find(key) == _map.end()) {
      return false;
    }

    weak_ptr<V> value = _map[key];

    return !value.expired();
  };

  void set(const K& key, const shared_ptr<V>& value) { _map[key] = weak_ptr<V>(value); };
};

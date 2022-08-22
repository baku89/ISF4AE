#include "WeakMap.hpp"

template <class K, class V>
shared_ptr<V> WeakMap<K, V>::get(const K &key) {
    
    if (!has(key)) {
        return nullptr;
    }
    
    return  _map[key].lock();
}

template <class K, class V>
bool WeakMap<K, V>::has(const K &key) {
    
    if (_map.find(key) == _map.end()) {
        return false;
    }
    
    weak_ptr<V> value = _map[key];
    
    return !value.expired();
}

template <class K, class V>
void WeakMap<K, V>::set(const K &key, const shared_ptr<V> &value) {
    
    _map[key] = weak_ptr<V>(value);
}

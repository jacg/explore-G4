#ifndef utils_map_set_hh
#define utils_map_set_hh

// helpers for map & set

template<class M, class K>
bool contains(M const& map, K const& key) { return map.find(key) != end(map); }

#endif

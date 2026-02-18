#include <unordered_set>
#include <algorithm> // for std::swap
template <typename T>
std::unordered_set<T> intersect(const std::unordered_set<T>& s1, const std::unordered_set<T>& s2);

template <typename T>
std::unordered_set<T> set_union(const std::unordered_set<T>& s1, const std::unordered_set<T>& s2);
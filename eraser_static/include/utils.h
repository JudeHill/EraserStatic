#pragma once
#include <unordered_set>
#include <algorithm> // for std::swap
template <typename T>
std::unordered_set<T> intersect(const std::unordered_set<T>& s1, const std::unordered_set<T>& s2) {
    // Optimization: Identify the smaller and larger sets
    const std::unordered_set<T>* smaller = &s1;
    const std::unordered_set<T>* larger = &s2;

    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    std::unordered_set<T> result;
    // Iterate over the smaller set to minimize lookups
    for (const auto& item : *smaller) {
        if (larger->find(item) != larger->end()) {
            result.insert(item);
        }
    }
    return result;
}

template <typename T>
std::unordered_set<T> set_union(const std::unordered_set<T>& s1, const std::unordered_set<T>& s2) {
    // Optimization: Identify smaller and larger sets
    const std::unordered_set<T>* smaller = &s1;
    const std::unordered_set<T>* larger = &s2;

    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    // Initialize result with the larger set to minimize re-hashing
    std::unordered_set<T> result = *larger;

    // Insert all elements from the smaller set
    // unordered_set::insert won't add duplicates
    result.insert(smaller->begin(), smaller->end());

    return result;
}
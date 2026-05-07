/* 
 * Project: EraserStatic
 * (https://github.com/JudeHill/EraserStatic)
 *
 * Copyright (C) 2025-2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


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
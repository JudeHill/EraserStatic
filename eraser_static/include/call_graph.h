/* 
 * This file was originally part of Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Pompay
 * Copyright (C) 2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This file was modified by Jude Hill in 2026 for use in EraserStatic.
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
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "usings.h"

class CallGraph {
public:
  explicit CallGraph();
  virtual ~CallGraph() = default;
  void addNode(std::string funcName, Filename fileName);
  void addEdge(std::string caller, std::string callee, bool onThread);
  std::vector<std::string>
  deltaLocksetOrdering(std::vector<std::string> functions);
  std::vector<std::string>
  functionVariableLocksetsOrdering(std::vector<std::string> functions);
  bool shouldVisitNode(std::string funcName);
  void markNodesAsStale(std::string fileName);
  void deleteStaleNodes();
  std::string getFilenameFromFuncname(std::string funcName);

private:
  struct FuncInfo{
    std::string funcname;
    Filename filename;
    bool stale;
    bool recently_changed;
    bool marked;
    int32_t indegree;
  };

  using Caller = std::string;
  using Callee = std::string;
  using CallInfo = u_int8_t;
  const CallInfo onThreadEdge = 1;
  const CallInfo notOnThreadEdge = 1 << 1;
  
  

  struct PairHash {
    std::size_t operator()(const std::pair<std::string, std::string>& p) const noexcept {
        auto h1 = std::hash<std::string>{}(p.first);
        auto h2 = std::hash<std::string>{}(p.second);

        // Combine hashes (standard technique)
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
  };

  std::unordered_map<std::pair<Caller, Callee>, CallInfo, PairHash> function_calls;
  std::unordered_map<std::string, FuncInfo> functions_table;
  std::vector<std::string> traverseGraph(bool reverse);
  std::vector<std::string> getNextNodes(std::vector<std::string> &order);
  void markNodes(std::vector<std::string> &startNodes, bool reverse);
};


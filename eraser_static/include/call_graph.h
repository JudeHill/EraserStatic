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


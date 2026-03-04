#pragma once
#include "basic_node.h"
#include "continue_node.h"
#include "continue_return_node.h"
#include "function_call_node.h"
#include "if_node.h"
#include "lock_node.h"
#include "parser.h"
#include "read_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "unlock_node.h"
#include "while_node.h"
#include "write_node.h"
#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>

enum VarStatus {
  VIRGIN,
  EXCLUSIVE,
  SHARED,
  SHARED_MODIFIED,

};

using LockName = std::string;
using FuncName = std::string;
using LockSet = std::unordered_set<LockName>;

struct VarInfo {
  std::string var_name;
  bool only_on_main;
  VarStatus status;
  LockSet lockset;
  int epoch;
};

enum RaceType {
  RACE_READ,
  RACE_WRITE,
};

struct DataRace {
  std::string var_name;
  GraphNode *node;
  RaceType race_type;
  LocationInfo location;
  unsigned int id;
};

using DataRaceMap = std::unordered_map<std::string, std::vector<DataRace>>;
using VarInfos = std::unordered_map<int, std::unique_ptr<VarInfo>>;

class Eraser {
private:
  FuncNodeMap start_nodes;
  std::vector<DataRace> data_races;
  std::unordered_map<std::string, VarInfos> vars;
  LockSet visit(GraphNode *node, LockSet lockset, std::unordered_set<FuncName> funcs_seen,
                bool on_main_thread = true, int epoch = 0);
  bool handle_read(LockName var_name, const LockSet lockset, bool on_main_thread, int epoch);
  bool handle_write(LockName var_name, const LockSet lockset, bool on_main_thread, int epoch);

public:
  Eraser() {};
  DataRaceMap compute_data_races(FuncNodeMap func_map, FuncName main_name = "main",
                                 bool debug_logging = false, bool symmetric_join = false);
};
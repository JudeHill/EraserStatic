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

enum class VarStatus {
  VIRGIN,
  EXCLUSIVE,
  SHARED,
  SHARED_MODIFIED,

};

using LockName = std::string;
using FuncName = std::string;
using LockSet = std::unordered_set<LockName>;

struct VarInfo {
  VarName var_name;
  bool only_on_main;
  VarStatus status;
  LockSet lockset;
  int epoch;
};

enum class RaceType {
  RACE_READ,
  RACE_WRITE,
};

using DataRaceID = unsigned int;
using Epoch = unsigned int;

struct DataRace {
  std::string var_name;
  GraphNode *node;
  RaceType race_type;
  LocationInfo location;
  DataRaceID id;
};

struct DataRaceMap {
  std::unordered_map<VarName, std::vector<std::shared_ptr<DataRace>>> by_var;
  std::unordered_map<DataRaceID, std::shared_ptr<DataRace>> by_id;
};

using VarInfos = std::unordered_map<Epoch, std::unique_ptr<VarInfo>>;

class Eraser {
private:
  FuncNodeMap start_nodes;
  std::vector<std::shared_ptr<DataRace>> data_races;
  std::unordered_map<VarName, VarInfos> vars;
  LockSet visit(GraphNode *node, LockSet lockset, std::unordered_set<FuncName> funcs_seen,
                bool on_main_thread = true, Epoch epoch = 0);
  bool handle_read(LockName var_name, const LockSet& lockset, bool on_main_thread, Epoch epoch);
  bool handle_write(LockName var_name, const LockSet& lockset, bool on_main_thread, Epoch epoch);

public:
  Eraser() {};
  std::shared_ptr<DataRaceMap> compute_data_races(FuncNodeMap func_map, FuncName main_name = "main",
                                 bool debug_logging = false, bool symmetric_join = false);
};
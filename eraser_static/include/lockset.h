#pragma once
#include "basic_node.h"
#include "lock_node.h"
#include "function_call_node.h"
#include "start_node.h"
#include "if_node.h"
#include "while_node.h"
#include "startwhile_node.h"
#include "continue_node.h"
#include "continue_return_node.h"
#include "read_node.h"
#include "parser.h"
#include "write_node.h"
#include "unlock_node.h"
#include <unordered_set>
#include <unordered_map>
#include <format>
#include <memory>



enum VarStatus {
    VIRGIN,
    EXCLUSIVE,
    SHARED,
    SHARED_MODIFIED,

};



using LockName = std::string;
using FuncName = std::string;
using LockSet = std::unordered_set<LockName>;


struct Var {
    std::string var_name;
    bool only_on_main;
    VarStatus status;
    LockSet lockset;
};


enum RaceType {
    RACE_READ,
    RACE_WRITE,
};

struct DataRace {
    std::string var_name;
    GraphNode* node;
    RaceType race_type;
    LocationInfo location;
};

using DataRaceMap = std::unordered_map<std::string, std::vector<DataRace>>;

class Eraser {
    private:
        FuncNodeMap start_nodes; 
        std::vector<DataRace> data_races; 
        std::unordered_map<std::string, std::unique_ptr<Var>> vars;
        LockSet visit(GraphNode *node, LockSet lockset, std::unordered_set<FuncName> funcs_seen, bool on_main_thread = true);
        bool handle_read(LockName var_name, const LockSet lockset, bool on_main_thread);
        bool handle_write(LockName var_name, const LockSet lockset, bool on_main_thread);
    public:
        Eraser(){};
        DataRaceMap compute_data_races(FuncNodeMap func_map, FuncName main_name = "main", bool debug_logging = false, bool symmetric_join = false);


};
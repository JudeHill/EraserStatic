#include "lockset.h"

static bool debug = true;
static WhileStack while_stack;
static int thread_depth = 0;
static bool assume_sym_join = false;
static DataRaceID next_race_id = 0;
LockSet intersect(LockSet lockset_1, const LockSet& lockset_2){
  for (auto it = lockset_1.begin(); it != lockset_1.end();) {
    if (!lockset_2.contains(*it)) {
      it = lockset_1.erase(it);
    } else {
      ++it;
    }
  }
  return lockset_1;
}

LockSet Eraser::visit(GraphNode *node, LockSet lockset, std::unordered_set<FuncName> funcs_seen, Context ctx) {
  if (node == nullptr) {
    if (debug) {
      std::cout << "Visited nullptr - returning" << std::endl;
    }
    return lockset;
  }
  if (debug) {
    std::cout << "Visiting node " << node->getPrintableNameWithId() << std::endl;
    std::string next_node_name = (node->getDefaultNextNode() == nullptr)
                                     ? "nullptr"
                                     : node->getDefaultNextNode()->getPrintableNameWithId();
    std::cout << "With next node " << next_node_name << std::endl;
  }
  switch (node->type) {
  case NodeType::LOCK: {
    LockNode *lock_node = static_cast<LockNode *>(node);
    LockSet new_lockset = lockset;
    new_lockset.insert(lock_node->varName);
    return visit(lock_node->next, new_lockset, funcs_seen, ctx);
  }
  case NodeType::FUNCTION_CALL: {
    FunctionCallNode *func_call_node = static_cast<FunctionCallNode *>(node);
    LockSet new_lockset = lockset;
    if (!funcs_seen.contains(func_call_node->functionName)) {
      // no recursion
      funcs_seen.insert(func_call_node->functionName);
      new_lockset = visit(start_nodes[func_call_node->functionName], lockset, funcs_seen,
                          ctx);
      funcs_seen.erase(func_call_node->functionName);
    }
    return visit(func_call_node->next, new_lockset, funcs_seen, ctx);
  }
  case NodeType::UNLOCK: {
    UnlockNode *unlock_node = static_cast<UnlockNode *>(node);
    LockSet new_lockset = lockset;
    new_lockset.erase(unlock_node->varName);
    return visit(unlock_node->next, new_lockset, funcs_seen, ctx);
  }

  case NodeType::READ: {
    ReadNode *read_node = static_cast<ReadNode *>(node);
    if (handle_read(read_node->varName, lockset, ctx.on_main_thread, ctx.epoch)) {
      data_races.emplace_back(std::make_shared<DataRace>(DataRace{
          .var_name = read_node->varName,
          .node = read_node,
          .race_type = RaceType::RACE_READ,
          .location = read_node->loc,
          .id = next_race_id,
      }));
      next_race_id++;
    }
    return visit(read_node->next, lockset, funcs_seen, ctx);
  }

  case NodeType::WRITE: {
    WriteNode *write_node = static_cast<WriteNode *>(node);
    if (handle_write(write_node->varName, lockset, ctx.on_main_thread, ctx.epoch)) {
      data_races.emplace_back(std::make_shared<DataRace>(DataRace{
          .var_name = write_node->varName,
          .node = write_node,
          .race_type = RaceType::RACE_WRITE,
          .location = write_node->loc,
          .id = next_race_id,
      }));
      next_race_id++;
    };
    return visit(write_node->next, lockset, funcs_seen, ctx);
  }

  case NodeType::IF: {
    IfNode *ifnode = static_cast<IfNode *>(node);
    // figure out what to do here
    GraphNode *else_node = ifnode->elseNode;
    GraphNode *if_node = ifnode->ifNode;
    LockSet if_lockset = visit(if_node, lockset, funcs_seen, ctx);
    LockSet else_lockset = visit(else_node, lockset, funcs_seen, ctx);
    for (auto it = if_lockset.begin(); it != if_lockset.end();) {
      if (!else_lockset.contains(*it)) {
        it = if_lockset.erase(it);
      } else {
        it++;
      }
    }
    EndifNode *end_if = ifnode->endIf;
    // assert v is now the end_if corresponding to the original if
    return visit(end_if->getDefaultNextNode(), if_lockset, funcs_seen, ctx);
  }
  case NodeType::BREAK:
  case NodeType::CONTINUE: {
    while_stack.back().push_back(lockset);
    return lockset;
  }
  case NodeType::ENDIF:
  case NodeType::ENDWHILE:
  

  case NodeType::WHILE: {
    WhileNode *while_node = static_cast<WhileNode *>(node);
    while_stack.push_back({ });
    Context new_ctx = ctx;
    new_ctx.in_loop = true;
    LockSet new_lockset = visit(while_node->whileNode, lockset, funcs_seen, new_ctx);
    // new_lockset = new_lockset INTERSECT lockset (worst case lockset)
    new_lockset = intersect(new_lockset, lockset);
    for (const auto& set : while_stack.back()){
      new_lockset = intersect(new_lockset, set);
    }
    EndwhileNode *end_while = while_node->endWhile;
    while_stack.pop_back();
    return visit(end_while->next, new_lockset, funcs_seen, ctx);
  }
  case NodeType::THREAD_CREATE: {
    ThreadCreateNode *create_node = static_cast<ThreadCreateNode *>(node);
    thread_depth++;
    Context new_ctx = {
      .on_main_thread = false,
      .in_loop = false,
      .epoch = ctx.epoch,
    };
    LockSet new_lockset =
        visit(start_nodes[create_node->functionName], lockset, funcs_seen, new_ctx);

    return visit(create_node->next, new_lockset, funcs_seen, ctx);
  }
  case NodeType::THREAD_JOIN: {
    thread_depth--;
    ThreadJoinNode *join_node = static_cast<ThreadJoinNode *>(node);
    return visit(join_node->next, lockset, funcs_seen, ctx);
  }
  Context new_ctx = ctx;
  // We ignore barriers in loops (epoch handling becomes too complicated)
  if (!ctx.in_loop){
    new_ctx.epoch++;
  }
  case NodeType::BARRIER: {
    return visit(node->getDefaultNextNode(), lockset, funcs_seen, new_ctx);
  }

  default: {
    return visit(node->getDefaultNextNode(), lockset, funcs_seen, ctx);
  }
  }
}

bool Eraser::handle_read(LockName var_name, const LockSet& lockset, bool on_main_thread, Epoch epoch) {
  // handle init
  if (thread_depth == 0) {
    return false;
  }
  
  std::cout << "Handling read for var " << var_name << " with thread depth " << thread_depth
            << std::endl;
  if (!vars.contains(var_name)) {
    vars[var_name][epoch] = std::make_unique<VarInfo>(VarInfo{
        .var_name = var_name,
        .only_on_main = on_main_thread,
        .written_to = false,
        .status = VarStatus::VIRGIN,
        .lockset = lockset,
    });
    return false;
  }
  VarInfos &var_infos = vars[var_name];
  if (!var_infos.contains(epoch)){
    var_infos[epoch] = std::make_unique<VarInfo>(VarInfo{
      .var_name = var_name,
      .only_on_main = on_main_thread,
      .written_to = false,
      .status = VarStatus::VIRGIN,
      .lockset = lockset,
  });
  }

  VarInfo &var = *var_infos[epoch];
  LockSet old_lockset = var.lockset;
  var.only_on_main = (var.only_on_main && on_main_thread);
  for (auto it = var.lockset.begin(); it != var.lockset.end();) {
    if (!lockset.contains(*it)) {
      it = var.lockset.erase(it);
    } else {
      ++it;
    }
  }
  if (var.status == VarStatus::VIRGIN && !var.only_on_main) {
    var.status = var.written_to ? VarStatus::SHARED_MODIFIED : VarStatus::SHARED;
  }
  if (var.status == VarStatus::SHARED_MODIFIED && var.lockset.empty()) {
    // reset var lockset if data race
    var.lockset = old_lockset;
    return true;
  }
  return false;
}

bool Eraser::handle_write(LockName var_name, const LockSet& lockset, bool on_main_thread,
                        Epoch epoch) {
  // handle init
  if (thread_depth == 0) {
    return false;
  }

  std::cout << "Handling write for var " << var_name << " with thread depth: " << thread_depth
            << std::endl;
  if (!vars.contains(var_name)) {
    vars[var_name][epoch] = std::make_unique<VarInfo>(VarInfo{
        .var_name = var_name,
        .only_on_main = on_main_thread,
        .written_to = true,
        .status = VarStatus::VIRGIN,
        .lockset = lockset,
    });
  }

  VarInfos &var_infos = vars[var_name];
  if (!var_infos.contains(epoch)){
    var_infos[epoch] = std::make_unique<VarInfo>(VarInfo{
      .var_name = var_name,
      .only_on_main = on_main_thread,
      .written_to = true,
      .status = VarStatus::VIRGIN,
      .lockset = lockset,
  });
  }
  VarInfo &var = *var_infos[epoch];
  LockSet old_lockset = var.lockset;
  var.only_on_main = (var.only_on_main && on_main_thread);
  var.written_to = true;

  if (!(var.status == VarStatus::SHARED_MODIFIED) && !var.only_on_main) {
    var.status = VarStatus::SHARED_MODIFIED;
  }
  for (auto it = var.lockset.begin(); it != var.lockset.end();) {
    if (!lockset.contains(*it)) {
      it = var.lockset.erase(it);
    } else {
      ++it;
    }
  }
  if (var.lockset.empty() && !var.only_on_main) {
    // reset variable lockset back after reporting data race here, to avoid many more FP elsewhere.
    var.lockset = old_lockset;
    return true;
  }
  return false;
}


std::shared_ptr<DataRaceMap> Eraser::compute_data_races(FuncNodeMap func_map, FuncName main_name, bool debug_logging,
                           bool symmetric_join) {
  data_races.clear();
  assume_sym_join = symmetric_join;
  debug = debug_logging;
  start_nodes = func_map;
  if (!func_map.contains(main_name)) {
    throw std::logic_error(
        std::format("Function CFG map does not contain main name: {}", main_name));
  }
  StartNode *start_node = func_map[main_name];
  visit(start_node, LockSet(), {});
  std::shared_ptr<DataRaceMap> data_races_map;
  data_races_map = std::make_shared<DataRaceMap>();
  for (auto &dr : data_races) {
    data_races_map->by_var[dr->var_name].push_back(dr);
    data_races_map->by_id[dr->id] = dr;
  }
  return data_races_map;
}
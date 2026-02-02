#include "lockset.h"

static bool debug = true;
static int thread_depth = 0;
static bool assume_sym_join = false;

LockSet Eraser::visit(GraphNode *node, LockSet lockset, std::unordered_set<FuncName> funcs_seen, bool on_main_thread){
    if (node == nullptr){
        if (debug){
            std::cout << "Visited nullptr - returning" << std::endl;
        }
        return lockset;
    }
    if (debug){
        std::cout << "Visiting node " << node->getPrintableNameWithId() << std::endl;
        std::string next_node_name = (node->getDefaultNextNode() == nullptr) ? "nullptr" : node->getDefaultNextNode()->getPrintableNameWithId();
        std::cout << "With next node " << next_node_name << std::endl;
    }
    switch (node->type)
    {
        case LOCK: {
            LockNode *lock_node = static_cast<LockNode*>(node);
            LockSet new_lockset = lockset;
            new_lockset.insert(lock_node->varName);
            return visit(lock_node->next, new_lockset, funcs_seen, on_main_thread);
        }
        case FUNCTION_CALL: {
            FunctionCallNode *func_call_node = static_cast<FunctionCallNode*>(node);
            LockSet new_lockset = lockset;
            if (!funcs_seen.contains(func_call_node->functionName)){
                // no recursion
                funcs_seen.insert(func_call_node->functionName);
                new_lockset = visit(start_nodes[func_call_node->functionName], lockset, funcs_seen, on_main_thread);
                funcs_seen.erase(func_call_node->functionName);
            }
            return visit(func_call_node->next, new_lockset, funcs_seen, on_main_thread);
            
        }
        case UNLOCK: {
            UnlockNode *unlock_node = static_cast<UnlockNode*>(node);
            LockSet new_lockset = lockset;
            new_lockset.erase(unlock_node->varName);
            return visit(unlock_node->next, new_lockset, funcs_seen, on_main_thread);
        }

        case READ: {
            ReadNode* read_node = static_cast<ReadNode*>(node);
            if (handle_read(read_node->varName, lockset, on_main_thread)){
                data_races.emplace_back(DataRace{
                    .var_name = read_node->varName,
                    .node = read_node,
                    .race_type = RACE_READ,
                    .location = read_node->loc,
                });
            }
            return visit(read_node->next, lockset, funcs_seen, on_main_thread);
        }

        case WRITE: {
            WriteNode* write_node = static_cast<WriteNode*>(node);
            if (handle_write(write_node->varName, lockset, on_main_thread)){
                data_races.emplace_back(DataRace{
                    .var_name = write_node->varName,
                    .node = write_node,
                    .race_type = RACE_WRITE,
                    .location = write_node->loc,
                });
            };
            return visit(write_node->next, lockset, funcs_seen, on_main_thread);
        }

        case IF: {
            IfNode* ifnode = static_cast<IfNode*>(node);
            // figure out what to do here
            GraphNode *else_node = ifnode->elseNode;
            GraphNode *if_node = ifnode->ifNode;
            LockSet if_lockset = visit(if_node, lockset, funcs_seen, on_main_thread);
            LockSet else_lockset = visit(else_node, lockset, funcs_seen, on_main_thread);
            for (auto it = if_lockset.begin();it != if_lockset.end();){
                if (!else_lockset.contains(*it)){
                    it = if_lockset.erase(it);
                } else {
                    it++;
                }
            }
            EndifNode* end_if = ifnode->endIf;
            // assert v is now the end_if corresponding to the original if
            return visit(end_if->getDefaultNextNode(), if_lockset, funcs_seen, on_main_thread);
            

            
        }   
        case ENDIF: 
        case ENDWHILE: 
        case CONTINUE:
        {
            return lockset;
        }

        case WHILE: {
            WhileNode* while_node = static_cast<WhileNode*>(node);
            LockSet new_lockset = visit(while_node->whileNode, lockset, funcs_seen, on_main_thread);
            // new_lockset = new_lockset INTERSECT lockset (worst case lockset)
            for (auto it = new_lockset.begin(); it != new_lockset.end(); ) {
                if (!lockset.contains(*it)) {
                    it = new_lockset.erase(it); 
                } else {
                    ++it;
                }
            }
            EndwhileNode* end_while = while_node->endWhile;
            return visit(end_while->next, new_lockset, funcs_seen, on_main_thread);
        }
        case THREAD_CREATE: {
            ThreadCreateNode* create_node = static_cast<ThreadCreateNode*>(node);
            thread_depth++;
            LockSet new_lockset = visit(start_nodes[create_node->functionName], lockset, funcs_seen, false);
            
            return visit(create_node->next, new_lockset, funcs_seen, on_main_thread);
        }
        case THREAD_JOIN: {
            thread_depth--;
            ThreadJoinNode* join_node = static_cast<ThreadJoinNode*>(node);
            return visit(join_node->next, lockset, funcs_seen, on_main_thread);
        }
        default: {
            return visit(node->getDefaultNextNode(), lockset, funcs_seen, on_main_thread);

        }
        
    }
}


bool Eraser::handle_read(LockName var_name, const LockSet lockset, bool on_main_thread){
    // handle init
    std::cout << "Handling read for var " << var_name << " with thread depth " << thread_depth << std::endl;
    if (!vars.contains(var_name)){
        vars[var_name] = std::make_unique<Var>(Var{
            .var_name = var_name,
            .only_on_main = on_main_thread,
            .status = VIRGIN,
            .lockset = lockset,
        });
        return false;
    }

    Var &var = *vars[var_name];
    if (thread_depth == 0){
        return false;
    }
    if (var.status == VIRGIN){
        var.lockset = lockset;
    }
    var.only_on_main = (var.only_on_main && on_main_thread);
    for (auto it = var.lockset.begin(); it != var.lockset.end(); ) {
        if (!lockset.contains(*it)) {
            it = var.lockset.erase(it);
        } else {
            ++it;
        }
    }
    if (var.status == VIRGIN && !var.only_on_main){
        var.status = SHARED;
    }
    if (var.status == SHARED_MODIFIED && var.lockset.empty()){
        return true;
    }
    return false;

}

bool Eraser::handle_write(LockName var_name, const LockSet lockset, bool on_main_thread){
    // handle init
    std::cout << "Handling write for var " << var_name << " with thread depth: " << thread_depth << std::endl;
    if (!vars.contains(var_name)){
        vars[var_name] = std::make_unique<Var>(Var{
            .var_name = var_name,
            .only_on_main = on_main_thread,
            .status = VIRGIN,
            .lockset = lockset,
        });
    }
    if (thread_depth == 0){
        return false;
    }
    

    Var &var =*vars[var_name];

    if (var.status == VIRGIN){
        var.lockset = lockset;
    }
    var.only_on_main = (var.only_on_main && on_main_thread);

    if (!(var.status == SHARED_MODIFIED) && !var.only_on_main){
        var.status = SHARED_MODIFIED;
    }
    for (auto it = var.lockset.begin(); it != var.lockset.end(); ) {
        if (!lockset.contains(*it)) {
            it = var.lockset.erase(it);
        } else {
            ++it;
        }
    }
    if (var.lockset.empty() && !var.only_on_main){
        return true;
    }
    return false;
}

std::unordered_map<std::string, std::vector<DataRace>> Eraser::compute_data_races(FuncNodeMap func_map, FuncName main_name, bool debug_logging, bool symmetric_join){
    data_races.clear();
    assume_sym_join = symmetric_join;
    debug = debug_logging;
    start_nodes = func_map;
    if (!func_map.contains(main_name)){
        throw std::logic_error(std::format("Function CFG map does not contain main name: {}", main_name));
    }
    StartNode* start_node = func_map[main_name];
    visit(start_node, LockSet(), {});
    std::unordered_map<std::string, std::vector<DataRace>> data_races_map;
    for (const auto& dr : data_races){
        data_races_map[dr.var_name].push_back(dr);
    }
    return data_races_map;
}
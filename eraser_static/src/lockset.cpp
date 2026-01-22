#include "lockset.h"

static bool debug = true;
static int thread_depth = 0;

LockSet Eraser::visit(GraphNode *node, LockSet lockset){
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
            return visit(lock_node->next, new_lockset);
        }
        case FUNCTION_CALL: {
            FunctionCallNode *func_call_node = static_cast<FunctionCallNode*>(node);
            LockSet new_lockset = visit(start_nodes[func_call_node->functionName], lockset);
            return visit(func_call_node->next, new_lockset);
            
        }
        case UNLOCK: {
            UnlockNode *unlock_node = static_cast<UnlockNode*>(node);
            LockSet new_lockset = lockset;
            new_lockset.erase(unlock_node->varName);
            return visit(unlock_node->next, new_lockset);
        }

        case READ: {
            ReadNode* read_node = static_cast<ReadNode*>(node);
            if (handle_read(read_node->varName, lockset)){
                data_races.emplace_back(DataRace{
                    .var_name = read_node->varName,
                    .node = read_node,
                });
            }
            return visit(read_node->next, lockset);
        }

        case WRITE: {
            WriteNode* write_node = static_cast<WriteNode*>(node);
            if (handle_write(write_node->varName, lockset)){
                data_races.emplace_back(DataRace{
                    .var_name = write_node->varName,
                    .node = write_node,
                });
            };
            return visit(write_node->next, lockset);
        }

        case IF: {
            IfNode* ifnode = static_cast<IfNode*>(node);
            // figure out what to do here
            GraphNode *else_node = ifnode->elseNode;
            GraphNode *if_node = ifnode->ifNode;
            LockSet if_lockset = visit(if_node, lockset);
            LockSet else_lockset = visit(else_node, lockset);
            for (auto it = if_lockset.begin();it != if_lockset.end();){
                if (!else_lockset.contains(*it)){
                    it = if_lockset.erase(it);
                } else {
                    it++;
                }
            }
            GraphNode* v = if_node;
            int count_nested_if = 1;
            while (count_nested_if > 0){
                if (v->type == ENDIF){
                    count_nested_if--;
                    if (count_nested_if == 0){
                        break;
                    }
                }
                if (v->type == IF){
                    count_nested_if++;
                    IfNode* new_if = static_cast<IfNode*>(v);
                    v = new_if->ifNode;
                } else {
                    v = v->getDefaultNextNode();
                }
                
            }
            // assert v is now the end_if corresponding to the original if
            return visit(v->getDefaultNextNode(), if_lockset);
            

            
        }   
        case ENDIF: 
        case ENDWHILE: 
        case CONTINUE:
        {
            return lockset;
        }

        case WHILE: {
            WhileNode* while_node = static_cast<WhileNode*>(node);
            LockSet new_lockset = visit(while_node->whileNode, lockset);
            // new_lockset = new_lockset INTERSECT lockset (worst case lockset)
            for (auto it = new_lockset.begin(); it != new_lockset.end(); ) {
                if (!lockset.contains(*it)) {
                    it = new_lockset.erase(it); 
                } else {
                    ++it;
                }
            }
            EndwhileNode* end_while = while_node->endWhile;
            return visit(end_while->next, new_lockset);
        }
        case THREAD_CREATE: {
            ThreadCreateNode* create_node = static_cast<ThreadCreateNode*>(node);
            thread_depth++;
            LockSet new_lockset = visit(start_nodes[create_node->functionName], lockset);
            
            return visit(create_node->next, new_lockset);
        }
        case THREAD_JOIN: {
            thread_depth--;
            ThreadJoinNode* join_node = static_cast<ThreadJoinNode*>(node);
            return visit(join_node->next, lockset);
        }
        default: {
            return visit(node->getDefaultNextNode(), lockset);

        }
        
    }
}


bool Eraser::handle_read(LockName var_name, const LockSet lockset){
    // handle init
    if (!vars.contains(var_name)){
        vars[var_name] = std::make_unique<Var>(Var{
            .var_name = var_name,
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
    for (auto lock : var.lockset){
        if (!lockset.contains(lock)){
            var.lockset.erase(lock);
        }
    }
    if (var.status == VIRGIN){
        var.status = SHARED;
    }
    if (var.status == SHARED_MODIFIED && var.lockset.empty()){
        return true;
    }
    return false;

}

bool Eraser::handle_write(LockName var_name, const LockSet lockset){
    // handle init
    if (!vars.contains(var_name)){
        vars[var_name] = std::make_unique<Var>(Var{
            .var_name = var_name,
            .status = VIRGIN,
            .lockset = lockset,
        });
        return false;
    }
    if (thread_depth == 0){
        return false;
    }
    

    Var &var =*vars[var_name];

    if (var.status == VIRGIN){
        var.lockset = lockset;
    }

    if (!(var.status == SHARED_MODIFIED)){
        var.status = SHARED_MODIFIED;
    }
    for (auto lock : var.lockset){
        if (!lockset.contains(lock)){
            var.lockset.erase(lock);
        }
    }
    if (var.lockset.empty()){
        return true;
    }
    return false;
}

std::vector<DataRace> Eraser::compute_data_races(FuncNodeMap func_map, FuncName main_name, bool debug_logging){
    data_races.clear();
    debug = debug_logging;
    start_nodes = func_map;
    if (!func_map.contains(main_name)){
        throw std::logic_error(std::format("Function CFG map does not contain main name: {}", main_name));
    }
    StartNode* start_node = func_map[main_name];
    visit(start_node, LockSet());
    return data_races;
}
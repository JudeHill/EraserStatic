#include "lockset.h"

static std::unordered_map<FuncName, StartNode*> start_nodes; 
static std::vector<DataRace> data_races;

LockSet visit(GraphNode *node, LockSet lockset){
    if (node == nullptr){
        return lockset;
    }
    switch (node->type)
    {
        case LOCK: {
            LockNode *lock_node = static_cast<LockNode*>(node);
            LockSet new_lockset = lockset;
            new_lockset.insert(lock_node->varName);
            return visit(lock_node->getNextNode(), new_lockset);
        }
        case FUNCTION_CALL: {
            FunctionCallNode *func_call_node = static_cast<FunctionCallNode*>(node);
            LockSet new_lockset = visit(start_nodes[func_call_node->functionName], lockset);
            return visit(node->getNextNode(), new_lockset);
            
        }
        case UNLOCK: {
            UnlockNode *unlock_node = static_cast<UnlockNode*>(node);
            LockSet new_lockset = lockset;
            new_lockset.erase(unlock_node->varName);
            return visit(node->getNextNode(), new_lockset);
        }

        case READ: {
            ReadNode* read_node = static_cast<ReadNode*>(node);
            if (handle_read(read_node->varName, lockset)){
                data_races.emplace_back(DataRace{
                    .var_name = read_node->varName,
                    .node = read_node,
                });
            }
            return visit(node->getNextNode(), lockset);
        }

        case WRITE: {
            WriteNode* write_node = static_cast<WriteNode*>(node);
            if (handle_write(write_node->varName, lockset)){
                data_races.emplace_back(DataRace{
                    .var_name = write_node->varName,
                    .node = write_node,
                });
            };
            return visit(node->getNextNode(), lockset);
        }

        case IF: {
            IfNode* node = static_cast<IfNode*>(node);
            // figure out what to do here
            GraphNode *else_node = node->elseNode;
            GraphNode *if_node = node->ifNode;
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
                v = v->getNextNode();
                if (v->type == IF){
                    count_nested_if++;
                } else if (v->type == ENDIF){
                    count_nested_if--;
                }
            }
            // assert v is now the end_if corresponding to the original if
            return visit(v->getNextNode(), if_lockset);
            // retrieve the end_if node somehow

            
        }   
        case ENDIF: 
        case ENDWHILE: 
        case CONTINUE:
        {
            return lockset;
        }

        case WHILE: {
            WhileNode* while_node = static_cast<WhileNode*>(node);
            LockSet new_lockset = visit(while_node->getNextNode(), lockset);
            // new_lockset = new_lockset INTERSECT lockset (worst case lockset)
            for (auto it = new_lockset.begin(); it != new_lockset.end(); ) {
                if (!lockset.contains(*it)) {
                    it = new_lockset.erase(it); 
                } else {
                    ++it;
                }
            }
            EndwhileNode* end_while = while_node->endWhile;
            return visit(end_while->getNextNode(), new_lockset);
        }
        default: {
            return visit(node->getNextNode(), lockset);

        }
        
    }
}


static std::unordered_map<std::string, std::unique_ptr<Var>> vars;

bool handle_read(LockName var_name, const LockSet lockset){
    // TODO
    if (!vars.contains(var_name)){
        throw std::logic_error(std::format("Attempted read to unrecognised variable name {}", var_name));
    }
    Var &var = *vars[var_name];
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

bool handle_write(LockName var_name, const LockSet lockset){
    // handle init
    if (!vars.contains(var_name)){
        vars[var_name] = std::make_unique<Var>(Var{
            .status = VIRGIN,
            .var_name = var_name,
            .lockset = lockset,
        });
        return;
    }

    Var &var =*vars[var_name];
    if (!var.status == SHARED_MODIFIED){
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
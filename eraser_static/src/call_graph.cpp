#pragma once

#include "call_graph.h"

CallGraph::CallGraph(){};

void CallGraph::addNode(std::string funcName, Filename fileName){
    std::cout << "Adding node " << funcName << " into " << fileName << std::endl;
    FuncInfo func_info;
    if (functions_table.contains(funcName)){
        func_info = functions_table[funcName];
        func_info.recently_changed = true;
        func_info.stale = false;
        func_info.filename = fileName;

    } else {
        func_info = { 
            .funcname = funcName,
            .filename = fileName,
            .stale =  false,
            .recently_changed = true,
            .marked = false,
            .indegree = 0,
         };

         
    }
    functions_table[funcName] = func_info;
}

void CallGraph::addEdge(std::string caller, std::string callee, bool onThread){
    std::cout << "Adding edge from " << caller << " to " << callee << ", with onthread=" << onThread << std::endl;
    // ignore recursive calls
    if (caller == callee){
        return;
    }
    std::pair key{caller, callee};
    // add callee to functions table
    if (!functions_table.contains(callee)){
        FuncInfo func_info {
            .funcname = callee,
            .filename = functions_table[caller].filename,
            .stale = false,
            .recently_changed = false,
            .marked = false,
            .indegree = 0,
        };
        functions_table[callee] = func_info;
    }
    // add (caller, callee, onThread) tuple to map. If it already exists, map is unaltered (|= is idempotent)
    if (!function_calls.contains(key)){
        CallInfo function_call_edge = onThread ? onThreadEdge : notOnThreadEdge;
        function_calls[key] = function_call_edge;
        return;
    }
    function_calls[key] |= (onThread ? onThreadEdge : notOnThreadEdge);
}

void CallGraph::markNodes(std::vector<std::string> &startNodes, 
    bool reverse = false){
    std::cout << "attempting to mark nodes" << std::endl;
    std::vector<std::string> q = {};

    for (const auto &node : startNodes) {
        q.push_back(node);
    }
}

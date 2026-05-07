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

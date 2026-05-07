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

#pragma once
#include "break_node.h"
#include "barrier_node.h"
#include "call_graph.h"
#include "construction_environment.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "eraser_ignore_off_node.h"
#include "eraser_ignore_on_node.h"
#include "file_includes.h"
#include "function_call_node.h"
#include "if_node.h"
#include "lock_node.h"
#include "parser_helpers.h"
#include "read_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "thread_create_node.h"
#include "thread_join_node.h"
#include "unlock_node.h"
#include "write_node.h"
#include <graph_visualizer.h>
#include <clang-c/Index.h>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

enum class BranchType {
  BRANCH_NONE,
  BRANCH_IF,
  BRANCH_ELSE_IF,
  BRANCH_ELSE,
  BRANCH_STARTWHILE,
  BRANCH_WHILE,
  BRANCH_DO_WHILE_START,
  BRANCH_DO_WHILE_COND,
  BRANCH_FOR_START,
  BRANCH_FOR_ITERATOR,
  BRANCH_FOR
};

enum class LhsType { LHS_NONE, LHS_WRITE, LHS_READ_AND_WRITE };

struct VariableInfo {
  int scopeDepth;
  int scopeNum;
  bool isStatic;
  bool isAtomic;
};

using FuncNodeMap = std::unordered_map<std::string, StartNode*>;

struct VisitorData {
  CallGraph *callGraph;
  unsigned int childIndex;
  std::vector<GraphNode *> nodesToAdd;
  LhsType lhsType;
};

inline std::ostream &operator<<(std::ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

extern std::unordered_map<std::string, StartNode *> funcCfgs;

class Parser {
public:
  explicit Parser(CallGraph *callGraph_, FileIncludes *fileIncludes_);
  virtual ~Parser();

  void parseFile(const char *fileName, bool ignore_barriers = false, bool verbose = false, bool fileChanged = true);
  void handleFunctionCall(CXCursor cursor, std::vector<GraphNode *> *nodesToAdd);
  void visualizeCFG();
  std::vector<std::string> getFunctions();
  FuncNodeMap getFunctionCfgs();


private:
  CallGraph *callGraph;
  FileIncludes *fileIncludes;
  std::string fileNameString;
  CXCursor ast = clang_getNullCursor();
  void dump_AST(CXCursor ast);
};
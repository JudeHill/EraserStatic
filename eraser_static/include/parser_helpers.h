/* 
 * Project: EraserStatic
 * (https://github.com/JudeHill/EraserStatic)
 *
 * Copyright (C) 2025-2026 Jude Hill <jude-stephen-hill@outlook.com>
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
#include <clang-c/Index.h>
#include <unordered_map>
#include <vector>
#include <optional>
#include <iostream>
#include "startwhile_node.h"

#include <construction_environment.h>

static std::unordered_map<unsigned, unsigned> parentChildCount;
static CXChildVisitResult countChildrenVisitor(CXCursor c, CXCursor parent, CXClientData data);
unsigned getCachedChildCount(CXCursor parent);

void handleForStmt(CXCursor ForStmt, ConstructionEnvironment *environment);
WhileNode* handleForStmtIncrement(CXCursor ForStmt, CXCursor cond, ConstructionEnvironment *environment);
void handleForStmtCond(CXCursor ForStmt, CXCursor cond, ConstructionEnvironment *environment);
CXCursor getFirstChild(CXCursor cursor);
CXCursor peelExpr(CXCursor c);
std::string getStartRoutineName(CXCursor call);

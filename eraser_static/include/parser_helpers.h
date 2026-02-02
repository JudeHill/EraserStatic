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
void handleForStmtNoIncrement(CXCursor ForStmt, ConstructionEnvironment *environment);
CXCursor getFirstChild(CXCursor cursor);
CXCursor peelExpr(CXCursor c);
std::string getStartRoutineName(CXCursor call);

/*
 * This file was originally part of Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Popay
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

#include "parser.h"

static std::unordered_map<std::string, bool> funcMap = {};
static std::vector<std::string> functions = {};
static std::set<std::string> functionDeclarations = {};
static std::vector<std::unordered_map<std::string, VariableInfo>> scopeStack = {};
static std::vector<unsigned int> scopeNums = {0};
static int inFunc = 0;
static int scopeDepth = 0;
static bool ignoreNextCompound = false;
static bool ignoreBarriers = false;
static std::string funcName = "";
static StartNode *startNode = nullptr;
static std::string startNodeFuncName;
static ConstructionEnvironment *environment;
static bool updateCallGraph;
static bool eraserIgnoreOn = false;
static std::vector<IfNode *> if_stack;

std::unordered_map<std::string, StartNode *> funcCfgs;

struct ForChildInfo {
  bool sawBody = false;
  bool emittedIter = false;
  // optional: track whether we saw any init/cond already
};

static std::vector<ForChildInfo> forStack;

Parser::Parser(CallGraph *callGraph_, FileIncludes *fileIncludes_)
    : callGraph(callGraph_), fileIncludes(fileIncludes_) {
  funcCfgs = {};
  environment = new ConstructionEnvironment();
}

FuncNodeMap Parser::getFunctionCfgs() { return funcCfgs; }

std::string getCursorFilename(CXCursor cursor) {
  CXSourceLocation location = clang_getCursorLocation(cursor);

  CXFile file;
  unsigned line, column, offset;
  clang_getFileLocation(location, &file, &line, &column, &offset);

  if (file == nullptr) {
    return "";
  }

  CXString filename = clang_getFileName(file);
  std::string result = clang_getCString(filename);
  clang_disposeString(filename);

  return result;
}

LhsType assignmentOperatorType(CXCursor parent) {
  CXCursorKind parentKind = clang_getCursorKind(parent);

  CXSourceRange range = clang_getCursorExtent(parent);
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(parent);

  CXToken *tokens = nullptr;
  unsigned int numTokens = 0;
  clang_tokenize(tu, range, &tokens, &numTokens);

  LhsType result = LhsType::LHS_NONE;

  for (unsigned int i = 0; i < numTokens; ++i) {
    CXString tokenSpelling = clang_getTokenSpelling(tu, tokens[i]);
    std::string token = clang_getCString(tokenSpelling);

    if (parentKind == CXCursor_BinaryOperator && token == "=") {
      result = LhsType::LHS_WRITE;
    } else if (parentKind == CXCursor_UnaryOperator && (token == "++" || token == "--")) {
      result = LhsType::LHS_READ_AND_WRITE;
    } else if (parentKind == CXCursor_CompoundAssignOperator) {
      result = LhsType::LHS_READ_AND_WRITE;
    }

    clang_disposeString(tokenSpelling);
  }

  clang_disposeTokens(tu, tokens, numTokens);

  return result;
}

VariableInfo findVariableInfo(std::string varName) {
  for (int i = scopeDepth; i >= 0; i--) {
    auto t = scopeStack[i].find(varName);
    if (t != scopeStack[i].end()) {
      return t->second;
    }
  }
  struct VariableInfo variableInfo;
  variableInfo.scopeDepth = 0;
  variableInfo.scopeNum = 0;
  variableInfo.isStatic = false;
  variableInfo.isAtomic = false;
  scopeStack[0].insert({varName, variableInfo});
  return variableInfo;
}

bool isSharedVar(struct VariableInfo variableInfo) {
  return !variableInfo.isAtomic && (variableInfo.isStatic || variableInfo.scopeDepth == 0);
}

bool isSharedVar(std::string varName) { return isSharedVar(findVariableInfo(varName)); }

std::string getVariableName(std::string varName, CXCursor cursor,
                            struct VariableInfo variableInfo) {
  if (variableInfo.isStatic || variableInfo.scopeDepth > 0) {
    std::string fileName = getCursorFilename(cursor);
    varName = fileName + " " + std::to_string(variableInfo.scopeDepth) + " " +
              std::to_string(variableInfo.scopeNum) + " " + varName;
  }
  return varName;
}

std::string getVariableName(std::string varName, CXCursor cursor) {
  return getVariableName(varName, cursor, findVariableInfo(varName));
}

std::string getNthArg(CXCursor cursor, int targetArg, bool isPtr = false) {
  struct ArgClientData {
    int targetArg;
    bool isPtr;
    int argNum;
    CXCursor *argCursor;
  };

  struct ArgClientData clientData = {targetArg, isPtr, 0, NULL};

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        struct ArgClientData *argClientData = reinterpret_cast<struct ArgClientData *>(clientData);

        if (argClientData->argNum == argClientData->targetArg) {
          argClientData->argCursor = &c;
          if (argClientData->isPtr && clang_getCursorKind(c) == CXCursor_UnaryOperator) {
            argClientData->isPtr = false;
            return CXChildVisit_Recurse;
          }
          return CXChildVisit_Break;
        }

        argClientData->argNum++;

        return CXChildVisit_Continue;
      },
      &clientData);

  // if (clientData.argNum >= targetArg && !clientData.isPtr) {
  if (clientData.argNum >= targetArg) {
    CXCursor argCursor = *clientData.argCursor;

    argCursor = peelExpr(argCursor);
    if (clang_getCursorKind(argCursor) == CXCursor_DeclRefExpr) {
      CXString argSpelling = clang_getCursorSpelling(argCursor);
      std::string result = clang_getCString(argSpelling);
      clang_disposeString(argSpelling);
      return result;
    }
  }
  return "";
}

std::string getFuncName(CXCursor cursor, std::string funcName) {
  auto func = funcMap.find(funcName);
  if (func != funcMap.end() && func->second) {
    std::string fileName = getCursorFilename(cursor);
    funcName = fileName + " " + funcName;
  }
  return funcName;
}

void classifyVariable(CXCursor cursor, LhsType lhsType, std::vector<GraphNode *> *nodesToAdd) {
  CXString varNameObj = clang_getCursorSpelling(cursor);
  std::string varName = clang_getCString(varNameObj);
  clang_disposeString(varNameObj);

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXType cursorType = clang_getCursorType(cursor);

  if (cursorKind == CXCursor_DeclRefExpr && cursorType.kind == CXType_FunctionProto) {
    return;
  }

  struct VariableInfo variableInfo;
  bool isDeclaration = cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_ParmDecl;

  CXString typeSpelling = clang_getTypeSpelling(cursorType);
  std::string typeString = clang_getCString(typeSpelling);
  if (isDeclaration) {
    variableInfo.isStatic = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
    variableInfo.isAtomic = typeString.find("_Atomic") != std::string::npos;
    variableInfo.scopeDepth = scopeDepth;
    variableInfo.scopeNum = scopeNums[scopeDepth];
    scopeStack[scopeDepth].insert({varName, variableInfo});
    return;
  } else {
    variableInfo = findVariableInfo(varName);
  }
  if (variableInfo.isAtomic || (!variableInfo.isStatic && variableInfo.scopeDepth > 0)) {
    return;
  }
  if (typeString.find("pthread_mutex_t") != std::string::npos) {
    clang_disposeString(typeSpelling);
    return;
  }
  clang_disposeString(typeSpelling);

  if (variableInfo.isStatic) {
    std::string fileName = getCursorFilename(cursor);
    varName = fileName + " " + std::to_string(variableInfo.scopeDepth) + " " +
              std::to_string(variableInfo.scopeNum) + " " + varName;
  }

  if (functionDeclarations.find(varName) == functionDeclarations.end()) {
    if (lhsType == LhsType::LHS_WRITE) {
      (*nodesToAdd).push_back(new WriteNode(varName, clang_getCursorLocation(cursor)));
    } else if (lhsType == LhsType::LHS_READ_AND_WRITE) {
      (*nodesToAdd).push_back(new ReadNode(varName, clang_getCursorLocation(cursor)));
      (*nodesToAdd).push_back(new WriteNode(varName, clang_getCursorLocation(cursor)));
    } else {
      environment->onAdd(new ReadNode(varName, clang_getCursorLocation(cursor)));
    }
  }
}

BranchType getBranchType(CXCursor cursor, CXCursor parent, unsigned int childIndex) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXCursorKind parentKind = clang_getCursorKind(parent);
  if (parentKind == CXCursor_IfStmt || parentKind == CXCursor_ConditionalOperator) {
    if (childIndex == 1) {
      return BranchType::BRANCH_IF;
    } else if (childIndex == 2 && cursorKind == CXCursor_IfStmt) {
      return BranchType::BRANCH_ELSE_IF;
    } else if (childIndex == 2) {
      return BranchType::BRANCH_ELSE;
    }
  } else if (parentKind == CXCursor_WhileStmt) {
    if (childIndex == 0) {
      return BranchType::BRANCH_STARTWHILE;
    } else if (childIndex == 1) {
      return BranchType::BRANCH_WHILE;
    }
  } else if (parentKind == CXCursor_ForStmt) {
    if (childIndex == 1) {
      return BranchType::BRANCH_FOR_START;
    } else if (childIndex == 2) {
      return BranchType::BRANCH_FOR_ITERATOR;
    } else if (childIndex == 3) {
      return BranchType::BRANCH_FOR;
    }
  } else if (parentKind == CXCursor_DoStmt) {
    if (childIndex == 0) {
      return BranchType::BRANCH_DO_WHILE_START;
    } else if (childIndex == 1) {
      return BranchType::BRANCH_DO_WHILE_COND;
    }
  }
  return BranchType::BRANCH_NONE;
}

static bool isStmtLike(CXCursorKind k) {
  switch (k) {
  case CXCursor_CompoundStmt:
  case CXCursor_IfStmt:
  case CXCursor_ForStmt:
  case CXCursor_WhileStmt:
  case CXCursor_DoStmt:
  case CXCursor_SwitchStmt:
  case CXCursor_ReturnStmt:
  case CXCursor_BreakStmt:
  case CXCursor_ContinueStmt:
  case CXCursor_GotoStmt:
  case CXCursor_LabelStmt:
  case CXCursor_CaseStmt:
  case CXCursor_DefaultStmt:
  case CXCursor_NullStmt:
    return true;
  default:
    return false;
  }
}

BranchType getBranchTypeGPT(CXCursor cursor, CXCursor parent, unsigned int childIndex) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXCursorKind parentKind = clang_getCursorKind(parent);

  if (parentKind == CXCursor_IfStmt || parentKind == CXCursor_ConditionalOperator) {
    if (childIndex == 1) {
      return BranchType::BRANCH_IF;
    } else if (childIndex == 2 && cursorKind == CXCursor_IfStmt) {
      return BranchType::BRANCH_ELSE_IF;
    } else if (childIndex == 2) {
      return BranchType::BRANCH_ELSE;
    }
  } else if (parentKind == CXCursor_WhileStmt) {
    if (childIndex == 0) {
      return BranchType::BRANCH_STARTWHILE;
    } else if (childIndex == 1) {
      return BranchType::BRANCH_WHILE;
    }
  } else if (parentKind == CXCursor_DoStmt) {
    if (childIndex == 0) {
      return BranchType::BRANCH_DO_WHILE_START;
    } else if (childIndex == 1) {
      return BranchType::BRANCH_DO_WHILE_COND;
    }
  } else if (parentKind == CXCursor_ForStmt) {

    ForChildInfo &info = forStack.back();
    unsigned int count_children = getCachedChildCount(parent);
    // BIG assumption:
    // We assume all for loops are either: for(;;){body}
    // or for(;cond;) {body}, or for(;cond;increment){body}
    // or for(init;cond;inc) {body}
    // This is not necessarily true, but is better than what we had before.
    if (count_children == 1) {
      // Special case: for(;;).
      // TODO: figure out how to handle this
      return BranchType::BRANCH_FOR;
    }
    if (childIndex == count_children - 1) {
      return BranchType::BRANCH_FOR;
    }
    if (count_children == 2) {
      return BranchType::BRANCH_FOR_START;
    }

    if (count_children == 3) {
      if (childIndex == 1) {
        return BranchType::BRANCH_FOR_START;
      }
    }

    if (count_children == 4) {
      if (childIndex == 1) {
        return BranchType::BRANCH_FOR_START;
      } else if (childIndex == 2) {
        return BranchType::BRANCH_FOR_ITERATOR;
      }
    }
  }
  return BranchType::BRANCH_NONE;
}

void onNewScope() {
  scopeDepth += 1;
  scopeStack.push_back(std::unordered_map<std::string, VariableInfo>());
  if (scopeDepth >= scopeNums.size()) {
    scopeNums.push_back(0);
  } else {
    scopeNums[scopeDepth] += 1;
  }
}

void handleFunctionCall(CXCursor cursor, std::vector<GraphNode *> *nodesToAdd,
                        CallGraph *callGraph) {
  std::string caller = funcName;
  std::string funcName = clang_getCString(clang_getCursorSpelling(cursor));
  if (funcName == "EraserIgnoreOff") {
    eraserIgnoreOn = false;
    environment->onAdd(new EraserIgnoreOffNode());
  } else if (funcName == "pthread_mutex_lock" || funcName == "pthread_mutex_unlock") {
    std::string spelling = getNthArg(cursor, 1, true);
    VariableInfo variableInfo = findVariableInfo(spelling);
    if (isSharedVar(variableInfo)) {
      std::string varName = getVariableName(spelling, cursor, variableInfo);
      if (funcName == "pthread_mutex_lock") {
        environment->onAdd(new LockNode(varName));
      } else if (funcName == "pthread_mutex_unlock") {
        environment->onAdd(new UnlockNode(varName));
      }
    }
  } else if (funcName == "pthread_join") {
    std::string spelling = getNthArg(cursor, 1);
    VariableInfo variableInfo = findVariableInfo(spelling);
    std::string varName = getVariableName(spelling, cursor, variableInfo);
    bool global = isSharedVar(variableInfo);
    // if (varName != "") {
    environment->onAdd(new ThreadJoinNode(varName, global));
    // }
  } else if (funcName == "pthread_barrier_wait" && !ignoreBarriers) {
    std::string spelling = getNthArg(cursor, 1);
    VariableInfo variableInfo = findVariableInfo(spelling);
    std::string varName = getVariableName(spelling, cursor, variableInfo);
    bool global = isSharedVar(variableInfo);
    environment->onAdd(new BarrierNode(varName, global));

  } else if (!eraserIgnoreOn) {
    if (funcName == "pthread_create") {
      std::cout << "Started pthreads create" << std::endl;
      std::string called = getStartRoutineName(cursor);
      std::cout << "calculated name" << std::endl;
      if (called != "") {
        std::string spelling = getNthArg(cursor, 1, true);
        VariableInfo variableInfo = findVariableInfo(spelling);
        std::string varName = getVariableName(spelling, cursor, variableInfo);
        std::string funcName = getFuncName(cursor, called);
        bool global = isSharedVar(variableInfo);
        environment->onAdd(new ThreadCreateNode(funcName, varName, global));
        if (global && varName != "") {
          environment->onAdd(new WriteNode(varName, clang_getCursorLocation(cursor)));
        }
        if (updateCallGraph) {
          callGraph->addEdge(caller, funcName, true);
        }
      } else {
        std::cout << "Unable to get pthread_create name" << std::endl;
      }
    } else if (funcName == "EraserIgnoreOn") {
      eraserIgnoreOn = true;
      environment->onAdd(new EraserIgnoreOnNode());
    } else if (funcName != "pthread_cond_wait" && funcName != "pthread_cond_broadcast") {
      funcName = getFuncName(cursor, funcName);
      (*nodesToAdd).push_back(new FunctionCallNode(funcName));
      if (updateCallGraph) {
        callGraph->addEdge(caller, funcName, false);
      }
    }
  }
}

CXChildVisitResult printVisitor(CXCursor cursor, CXCursor parent, CXClientData data) {
  unsigned indent = *(unsigned *)data;
  for (unsigned i = 0; i < indent; ++i)
    std::cout << "  ";

  CXString kind = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
  CXString spelling = clang_getCursorSpelling(cursor);

  std::cout << clang_getCString(kind) << ": " << clang_getCString(spelling) << std::endl;

  clang_disposeString(kind);
  clang_disposeString(spelling);

  unsigned nextIndent = indent + 1;
  clang_visitChildren(cursor, printVisitor, &nextIndent);
  return CXChildVisit_Continue;
}

void Parser::dump_AST(CXCursor ast) {
  if (clang_Cursor_isNull(ast)) {
    std::cout << "Null cursor, exiting" << std::endl;
    return;
  }
  std::cout << "Non-null cursor, dumping" << std::endl;
  unsigned indent = 0;
  clang_visitChildren((ast), printVisitor, &indent);
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData) {

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  VisitorData *visitorData = reinterpret_cast<VisitorData *>(clientData);
  CallGraph *callGraph = visitorData->callGraph;

  if (cursorKind == CXCursor_ForStmt) {
    forStack.push_back(ForChildInfo{});
    // STUFF GOES HERE
    handleForStmt(cursor, environment);
  }

  if (cursorKind == CXCursor_FunctionDecl) {
    ignoreNextCompound = true;
    onNewScope();
    inFunc = 1;
    funcName = clang_getCString(clang_getCursorSpelling(cursor));
    bool isStatic = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
    funcMap.insert({funcName, isStatic});
    if (isStatic) {
      std::string fileName = getCursorFilename(cursor);
      funcName = fileName + " " + funcName;
    }
    functionDeclarations.insert(funcName);
  } else if (cursorKind == CXCursor_CompoundStmt) {
    if (ignoreNextCompound) {
      startNode = environment->startNewTree(funcName);
      startNodeFuncName = funcName;
      if (updateCallGraph) {
        callGraph->addNode(funcName, getCursorFilename(cursor));
      }
      ignoreNextCompound = false;
    } else {
      onNewScope();
    }
  }

  unsigned int childIndex = visitorData->childIndex;
  LhsType lhsType = visitorData->lhsType;
  LhsType nextLhsType = lhsType;
  if (lhsType == LhsType::LHS_NONE) {
    nextLhsType = assignmentOperatorType(cursor);
  }

  VisitorData childData = {callGraph, 0, {}, nextLhsType};
  std::vector<GraphNode *> nodesToAddAfterChildren = {};
  if (cursorKind == CXCursor_CallExpr) {
    handleFunctionCall(cursor, &childData.nodesToAdd, callGraph);
  } else if (cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_DeclRefExpr ||
             cursorKind == CXCursor_ParmDecl) {
    classifyVariable(cursor, lhsType, &visitorData->nodesToAdd);
  } else if (cursorKind == CXCursor_BreakStmt) {
    environment->onAdd(new BreakNode());
  } else if (cursorKind == CXCursor_ContinueStmt) {
    environment->onAdd(new ContinueNode());
  }

  BranchType branchType = getBranchTypeGPT(cursor, parent, childIndex);
  WhileNode *forNodeLoop = nullptr;

  if (branchType == BranchType::BRANCH_IF) {
    IfNode *if_node = new IfNode();
    if_stack.push_back(if_node);
    environment->onAdd(if_node);
  } else if (branchType == BranchType::BRANCH_ELSE_IF || branchType == BranchType::BRANCH_ELSE) {
    environment->onElseAdd();
  } else if (branchType == BranchType::BRANCH_STARTWHILE) {
    environment->onAdd(new StartwhileNode());
  } else if (branchType == BranchType::BRANCH_WHILE) {
    environment->onAdd(new WhileNode());
  } else if (branchType == BranchType::BRANCH_DO_WHILE_START) {
    StartwhileNode *startwhileNode = new StartwhileNode();
    startwhileNode->continueReturn = nullptr;
    startwhileNode->isDoWhile = branchType == BranchType::BRANCH_DO_WHILE_START;
    environment->onAdd(startwhileNode);
  } else if (branchType == BranchType::BRANCH_FOR_START) {
    handleForStmtCond(parent, cursor, environment);
  } else if (branchType == BranchType::BRANCH_DO_WHILE_COND) {
    std::cout << "do while" << std::endl;
    environment->onAdd(new ContinueReturnNode());
  } else if (branchType == BranchType::BRANCH_FOR_ITERATOR) {
    forNodeLoop = handleForStmtIncrement(parent, cursor, environment);
  }
  // RECURSIVE CALL
  clang_visitChildren(cursor, visitor, &childData);
  // ---------
  if (lhsType == LhsType::LHS_NONE) {
    for (int i = 0; i < childData.nodesToAdd.size(); i++) {
      environment->onAdd(childData.nodesToAdd[i]);
    }
  } else {
    for (int i = 0; i < childData.nodesToAdd.size(); i++) {
      visitorData->nodesToAdd.push_back(childData.nodesToAdd[i]);
    }
  }
  if (cursorKind == CXCursor_IfStmt || cursorKind == CXCursor_ConditionalOperator) {
    EndifNode *end_if = new EndifNode();
    if (if_stack.empty()) {
      throw new std::logic_error("Tried to add an end_if without a preceding if");
    }
    if_stack.back()->endIf = end_if;
    if_stack.pop_back();
    environment->onAdd(end_if);
  }
  if (branchType == BranchType::BRANCH_WHILE) {
    environment->onAdd(new ContinueNode());
    environment->onAdd(new EndwhileNode());
  }
  if (cursorKind == CXCursor_ReturnStmt) {
    environment->onAdd(new ReturnNode());
  }
  if (branchType == BranchType::BRANCH_DO_WHILE_START) {
    environment->onAdd(new ContinueNode());
  } else if (branchType == BranchType::BRANCH_DO_WHILE_COND) {
    WhileNode *whileNode = new WhileNode();
    whileNode->isDoWhile = true;
    environment->onAdd(whileNode);
    environment->onAdd(new EndwhileNode());
  } else if (branchType == BranchType::BRANCH_FOR_ITERATOR) {
    environment->goBackToStartWhile();
    environment->currNode = forNodeLoop;
  } else if (branchType == BranchType::BRANCH_FOR) {
    environment->onAdd(new ContinueNode());
    environment->onAdd(new EndwhileNode());
  }
  if (cursorKind == CXCursor_CompoundStmt) {
    scopeStack.pop_back();
    scopeDepth -= 1;
  }
  if (cursorKind == CXCursor_FunctionDecl) {
    if (ignoreNextCompound) {
      scopeStack.pop_back();
      scopeDepth -= 1;
    }
    ignoreNextCompound = false;
    inFunc = 0;
    if (startNode != nullptr) {
      if (funcName == startNodeFuncName) {
        environment->onAdd(new ReturnNode());
        functions.push_back(funcName);
        std::cout << "Adding " << funcName << " to the cfg" << std::endl;
        funcCfgs.insert({funcName, startNode});
        startNode = nullptr;
      } else {
        // declaration of a different function inside a function
        funcName = startNodeFuncName;
      }
    }
  }
  if (cursorKind == CXCursor_ForStmt) {
    forStack.pop_back();
  }

  visitorData->childIndex += 1;
  visitorData->lhsType = LhsType::LHS_NONE;
  return CXChildVisit_Continue;
}

void Parser::parseFile(const char *fileName, bool ignore_barriers, bool verbose, bool fileChanged) {
  funcMap = {};
  functionDeclarations = {};
  scopeStack.clear();
  scopeNums = {0};
  inFunc = 0;
  scopeDepth = 0;
  ignoreNextCompound = false;
  funcName = "";
  startNode = nullptr;
  ignoreBarriers = ignore_barriers;
  updateCallGraph = fileChanged;

  scopeStack.push_back(std::unordered_map<std::string, VariableInfo>());

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit =
      clang_parseTranslationUnit(index, fileName, nullptr, 0, nullptr, 0, CXTranslationUnit_None);

  if (unit == nullptr) {
    std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);

  VisitorData initialData = {callGraph, 0, {}, LhsType::LHS_NONE};

  clang_visitChildren(cursor, visitor, &initialData);

  if (fileChanged) {
    this->fileNameString = fileName;
    fileIncludes->clearIncludes(fileNameString);
    clang_getInclusions(
        unit,
        [](CXFile includedFile, CXSourceLocation *_includer, unsigned int _isIncluderNonlocal,
           CXClientData data) {
          CXString includedFileName = clang_getFileName(includedFile);
          std::string fileName = clang_getCString(includedFileName);
          clang_disposeString(includedFileName);
          Parser *parser = reinterpret_cast<Parser *>(data);
          if (parser->fileNameString != fileName &&
              fileName.find("/usr/include/") == std::string::npos &&
              fileName.find("/usr/lib/clang/") == std::string::npos) {
            parser->fileIncludes->addInclude(parser->fileNameString, fileName);
          }
        },
        this);
  }
  if (verbose) {
    dump_AST(cursor);
  }
  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}

std::vector<std::string> Parser::getFunctions() { return functions; }

void deallocateCFG(StartNode *node) {
  std::set<GraphNode *> nodes = {node};
  std::vector<GraphNode *> stack = {node};
  while (stack.size() > 0) {
    GraphNode *currNode = stack.back();
    stack.pop_back();
    std::vector<GraphNode *> nextNodes = currNode->getNextNodes();
    for (int i = 0; i < nextNodes.size(); i++) {
      GraphNode *nextNode = nextNodes[i];
      if (nodes.find(nextNode) != nodes.end()) {
        nodes.insert(nextNode);
        stack.push_back(nextNode);
      }
    }
  }
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    delete *it;
  }
}

Parser::~Parser() {
  delete environment;
  for (auto it = funcCfgs.begin(); it != funcCfgs.end(); ++it) {
    deallocateCFG(it->second);
  }
}

void Parser::visualizeCFG() {
  GraphVisualizer *gv = new GraphVisualizer();
  for (const auto &[_, node] : funcCfgs) {
    gv->visualizeGraph(node);
  }

  delete gv;
}

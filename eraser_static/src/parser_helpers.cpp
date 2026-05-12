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


#include "parser_helpers.h"
  
  static unsigned countChildren(CXCursor parent) {
    unsigned n = 0;
    clang_visitChildren(parent, countChildrenVisitor, &n);
    return n;
  }


unsigned getCachedChildCount(CXCursor parent) {
  unsigned h = clang_hashCursor(parent);
  auto it = parentChildCount.find(h);
  if (it != parentChildCount.end()) return it->second;
  unsigned n = countChildren(parent);
  parentChildCount[h] = n;
  return n;
}


enum class ForClauseKind { INIT, COND, INC, BODY, UNKNOWN };

struct ForChild {
  CXCursor cursor;
  unsigned childIndex; // index among *direct* children of ForStmt
  ForClauseKind kind;
};

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

void handleForStmt(CXCursor ForStmt, ConstructionEnvironment *environment){
  unsigned num_children = getCachedChildCount(ForStmt);
  std::cout << "Child count: " << num_children << std::endl;
  if (num_children == 1){
    // for(;;)
    // start while
    StartwhileNode *startwhileNode = new StartwhileNode();
    startwhileNode->continueReturn = nullptr;
    startwhileNode->isDoWhile = false;
    WhileNode* forNodeLoop = new WhileNode();
    startwhileNode->continueReturn = forNodeLoop;
    environment->onAdd(startwhileNode);
    // normal while )
    
    environment->onAdd(forNodeLoop);
  }
}

void handleForStmtCond(CXCursor ForStmt, CXCursor cond, ConstructionEnvironment *environment){
   // start while
   StartwhileNode *startwhileNode = new StartwhileNode();
   startwhileNode->continueReturn = nullptr;
   startwhileNode->isDoWhile = false;
   if (getCachedChildCount(ForStmt) < 4){
    // assume no increment
    std::cout << "Handling child count " << getCachedChildCount(ForStmt) << std::endl;
    WhileNode* forNodeLoop = new WhileNode();
    startwhileNode->continueReturn = forNodeLoop;
    environment->onAdd(forNodeLoop);
   }
   environment->onAdd(startwhileNode);
}


WhileNode* handleForStmtIncrement(CXCursor ForStmt, CXCursor increment, ConstructionEnvironment *environment){
    WhileNode* forNodeLoop = new WhileNode();
    environment->onAdd(forNodeLoop);
    environment->onAdd(new ContinueReturnNode());
    return forNodeLoop;
}

static bool isExprLike(CXCursorKind k) {
  // Keep this permissive; you mainly want to avoid TypeRef/TemplateRef/etc.
  switch (k) {
    case CXCursor_TypeRef:
    case CXCursor_TemplateRef:
    case CXCursor_NamespaceRef:
    case CXCursor_MemberRef:
    case CXCursor_LabelRef:
    case CXCursor_OverloadedDeclRef:
      return false;
    default:
      return true;
  }
}

static CXCursor getFirstExprChild(CXCursor c) {
  struct Data { CXCursor out; };
  Data d{ clang_getNullCursor() };

  clang_visitChildren(
    c,
    [](CXCursor ch, CXCursor, CXClientData data) {
      auto *d = reinterpret_cast<Data*>(data);
      if (clang_Cursor_isNull(d->out) && isExprLike(clang_getCursorKind(ch))) {
        d->out = ch;
        return CXChildVisit_Break;
      }
      return CXChildVisit_Continue;
    },
    &d
  );

  return d.out;
}


CXCursor peelExpr(CXCursor c) {
  while (true) {
    CXCursorKind k = clang_getCursorKind(c);
    if (k == CXCursor_ParenExpr ||
        k == CXCursor_UnaryOperator || 
        k == CXCursor_UnexposedExpr ||
        k == CXCursor_CStyleCastExpr ||
        k == CXCursor_CXXFunctionalCastExpr ||
        k == CXCursor_CXXStaticCastExpr ||
        k == CXCursor_CXXReinterpretCastExpr ||
        k == CXCursor_CXXConstCastExpr) {
      CXCursor child = getFirstExprChild(c);
      if (clang_Cursor_isNull(child)) break;
      c = child;
      continue;
    }
    break;
  }
  return c;
}

// search for the function term within the mess of cursor casts
CXCursor findFunctionDeclRef(CXCursor root) {
  struct Data { CXCursor out; };
  Data d{ clang_getNullCursor() };

  clang_visitChildren(
    root,
    [](CXCursor ch, CXCursor, CXClientData data) {
      auto *d = reinterpret_cast<Data*>(data);

      if (clang_getCursorKind(ch) == CXCursor_DeclRefExpr) {
        CXCursor ref = clang_getCursorReferenced(ch);
        if (!clang_Cursor_isNull(ref) &&
            clang_getCursorKind(ref) == CXCursor_FunctionDecl) {
          d->out = ch;
          return CXChildVisit_Break;
        }
      }
      return CXChildVisit_Recurse;
    },
    &d
  );

  return d.out;
}

std::string getStartRoutineName(CXCursor call) {
  if (clang_Cursor_getNumArguments(call) < 3) return "";
  CXCursor arg2 = clang_Cursor_getArgument(call, 2);

  CXCursor declRef = findFunctionDeclRef(arg2);
  if (clang_Cursor_isNull(declRef)) return "";

  CXString s = clang_getCursorSpelling(declRef);
  std::string name = clang_getCString(s);
  clang_disposeString(s);
  return name;
}




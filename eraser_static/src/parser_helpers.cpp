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


static CXChildVisitResult countChildrenVisitor(CXCursor c, CXCursor parent, CXClientData data) {
    auto *n = static_cast<unsigned*>(data);
    (*n)++;
    return CXChildVisit_Continue; // don't recurse further
  }
  
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

// Walk subtree and see if it contains a declaration statement.
static bool subtreeHasDeclStmt(CXCursor root) {
  struct Data { bool found = false; };
  Data data;

  clang_visitChildren(
      root,
      [](CXCursor c, CXCursor /*parent*/, CXClientData clientData) {
        auto *d = static_cast<Data*>(clientData);
        if (clang_getCursorKind(c) == CXCursor_DeclStmt ||
            clang_getCursorKind(c) == CXCursor_VarDecl) {
          d->found = true;
          return CXChildVisit_Break;
        }
        return CXChildVisit_Recurse;
      },
      &data
  );

  return data.found;
}

// Heuristic: "looks like increment" if subtree contains ++/-- or assignment-ish ops.
static bool subtreeLooksLikeIncrement(CXCursor root) {
  struct Data { bool found = false; };
  Data data;

  clang_visitChildren(
      root,
      [](CXCursor c, CXCursor /*parent*/, CXClientData clientData) {
        auto *d = static_cast<Data*>(clientData);
        CXCursorKind k = clang_getCursorKind(c);

        // ++/-- are UnaryOperator nodes (spelling can distinguish but we avoid tokenizing),
        // still a strong hint if present.
        if (k == CXCursor_UnaryOperator ||
            k == CXCursor_CompoundAssignOperator ||
            k == CXCursor_BinaryOperator) {
          // We can't know which operator for BinaryOperator without tokens,
          // but assignment forms are very common in inc; treat presence as a signal.
          d->found = true;
          return CXChildVisit_Break;
        }

        return CXChildVisit_Recurse;
      },
      &data
  );

  return data.found;
}

// Collect direct children of a cursor (non-recursive).
static std::vector<CXCursor> getDirectChildren(CXCursor parent) {
  struct Data { std::vector<CXCursor> out; };
  Data data;

  clang_visitChildren(
      parent,
      [](CXCursor c, CXCursor /*p*/, CXClientData clientData) {
        auto *d = static_cast<Data*>(clientData);
        d->out.push_back(c);
        return CXChildVisit_Continue; // do not recurse; direct children only
      },
      &data
  );

  return data.out;
}

// Main helper: classify ForStmt children as INIT/COND/INC/BODY.
// This is "good enough" and avoids tokenization.
static std::vector<ForChild> classifyForStmtChildrenGoodEnough(CXCursor forCursor) {
  std::vector<ForChild> result;

  if (clang_getCursorKind(forCursor) != CXCursor_ForStmt) {
    return result;
  }

  auto children = getDirectChildren(forCursor);

  // Find body: first stmt-like direct child (usually last, but first is safe).
  std::optional<unsigned> bodyIdx;
  for (unsigned i = 0; i < children.size(); ++i) {
    CXCursorKind k = clang_getCursorKind(children[i]);
    if (isStmtLike(k)) {
      bodyIdx = i;
      break;
    }
  }

  // Build header list (direct children before body).
  std::vector<unsigned> headerIndices;
  if (bodyIdx.has_value()) {
    for (unsigned i = 0; i < *bodyIdx; ++i) headerIndices.push_back(i);
  } else {
    // Degenerate (shouldn't happen): treat all as header.
    for (unsigned i = 0; i < children.size(); ++i) headerIndices.push_back(i);
  }

  // Convenience lambdas
  auto setKind = [&](unsigned idx, ForClauseKind kind) {
    result.push_back(ForChild{children[idx], idx, kind});
  };

  // First, mark body (if present).
  if (bodyIdx.has_value()) {
    setKind(*bodyIdx, ForClauseKind::BODY);
  }

  // Now classify headers with the heuristic you asked for.
  const size_t h = headerIndices.size();

  if (h == 3) {
    setKind(headerIndices[0], ForClauseKind::INIT);
    setKind(headerIndices[1], ForClauseKind::COND);
    setKind(headerIndices[2], ForClauseKind::INC);
  } else if (h == 2) {
    unsigned a = headerIndices[0];
    unsigned b = headerIndices[1];

    bool aIsDecl = subtreeHasDeclStmt(children[a]) ||
                   clang_getCursorKind(children[a]) == CXCursor_DeclStmt;
    bool bLooksInc = subtreeLooksLikeIncrement(children[b]);

    if (aIsDecl) {
      // init + (cond or inc)
      setKind(a, ForClauseKind::INIT);
      setKind(b, bLooksInc ? ForClauseKind::INC : ForClauseKind::COND);
    } else if (bLooksInc) {
      // (init/cond) + inc → assume second is inc
      setKind(b, ForClauseKind::INC);

      // First: if it looks assignment/decl-ish treat as init else cond
      bool aLooksIncOrInit = subtreeLooksLikeIncrement(children[a]) || aIsDecl;
      setKind(a, aLooksIncOrInit ? ForClauseKind::INIT : ForClauseKind::COND);
    } else {
      // Default: second is condition; first is init if assignment/decl-ish else cond.
      setKind(b, ForClauseKind::COND);
      bool aLooksInit = aIsDecl || subtreeLooksLikeIncrement(children[a]);
      setKind(a, aLooksInit ? ForClauseKind::INIT : ForClauseKind::COND);
    }
  } else if (h == 1) {
    unsigned a = headerIndices[0];
    bool aIsDecl = subtreeHasDeclStmt(children[a]) ||
                   clang_getCursorKind(children[a]) == CXCursor_DeclStmt;
    bool aLooksInc = subtreeLooksLikeIncrement(children[a]);

    if (aIsDecl) setKind(a, ForClauseKind::INIT);
    else if (aLooksInc) setKind(a, ForClauseKind::INC);
    else setKind(a, ForClauseKind::COND);
  } else {
    // h == 0 or >3: uncommon/odd; mark unknown so caller can fall back.
    for (unsigned idx : headerIndices) setKind(idx, ForClauseKind::UNKNOWN);
  }

  return result;
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

CXCursor getFirstChild(CXCursor cursor) {
  CXCursor firstChild = clang_getNullCursor();

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        CXCursor *firstChildPtr = reinterpret_cast<CXCursor *>(clientData);

        *firstChildPtr = c;
        return CXChildVisit_Break;
      },
      &firstChild);

  return firstChild;
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




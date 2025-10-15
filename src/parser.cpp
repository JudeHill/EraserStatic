#include "eraser_static/parser.h"
#include <functional>

static CXChildVisitResult printVisitor(CXCursor cursor, CXCursor parent, CXClientData data) {
    unsigned indent = *(unsigned*)data;
    for (unsigned i = 0; i < indent; ++i) std::cout << "  ";

    CXString kind = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
    CXString spelling = clang_getCursorSpelling(cursor);

    std::cout << clang_getCString(kind) << ": " << clang_getCString(spelling) << std::endl;

    clang_disposeString(kind);
    clang_disposeString(spelling);

    unsigned nextIndent = indent + 1;
    clang_visitChildren(cursor, printVisitor, &nextIndent);
    return CXChildVisit_Continue;
}


Parser::Parser(const std::string file) : filepath(file) {}


void Parser::Parse(){
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
    index, this->filepath.c_str(), nullptr, 0, nullptr, 0, CXTranslationUnit_None);
    
    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        exit(-1);
    }

    CXCursor ast_root = clang_getTranslationUnitCursor(unit);
    this->ast = ast_root;

}

void Parser::dump_AST(){

    unsigned indent = 0;
    clang_visitChildren((this->ast), printVisitor, &indent);
}

    

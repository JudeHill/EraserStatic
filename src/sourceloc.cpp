#include "sourceloc.hpp"
#include <clang/AST/DeclBase.h>
#include "clang/Basic/SourceManager.h"

SourceLoc toSpellingLoc(const clang::SourceManager &SM, const clang::SourceLocation &loc) {
    SourceLoc out;
    if (loc.isInvalid()){
        return out;
    }

    clang::SourceLocation spelling = SM.getSpellingLoc(loc);
    clang::SourceLocation expansion = SM.getExpansionLoc(loc);
    
    const clang::FileEntry *FE = SM.getFileEntryForID(SM.getFileID(spelling));
    if (FE){
        out.file = FE->getName().str();
    }

    out.line = SM.getSpellingLineNumber(spelling);
    out.column = SM.getSpellingColumnNumber(spelling);

    out.in_macro = loc.isMacroID();
    if (out.in_macro){
        const clang::FileEntry *ExpFile = SM.getFileEntryForID(SM.getFileID(expansion));
        if (ExpFile) {
            out.expansion_file = ExpFile->getName().str();
    
        }
    }
}
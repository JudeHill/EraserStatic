#include "symbols.hpp"
#include "clang/Index/USRGeneration.h"
using namespace clang;

std::string qualName(const clang::NamedDecl* named_decl){
    const std::string name = named_decl->getQualifiedNameAsString();
    if (name.empty()){
        if (llvm::isa<clang::RecordDecl>(named_decl)){
            return "<anonymous-records>";
        }
        if (llvm::isa<clang::NamespaceDecl>(named_decl)){
            return "<anonymous-namespace>";
        }
        if (llvm::isa<clang::VarDecl>(named_decl)){
            return "<anonymous-var>";
        }
        return "<unnamed>";
    }
    return name;

}

std::string usr(const clang::Decl* decl, clang::ASTContext& ast_context){
    llvm::SmallVector<char, 128> usr;
    if (clang::index::generateUSRForDecl(decl, usr)){
        return "<invalid-usr>";
    };

    return "";
}
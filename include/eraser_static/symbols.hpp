#include <string>

#include <clang/AST/Decl.h> // Include the appropriate header for clang::Decl


// produce a human-readable name for a declaration 
// (function, variable, method, type, field)
std::string qualName(const clang::NamedDecl*);


// machine name - guarunteed unique
std::string usr(const clang::Decl*, clang::ASTContext&);
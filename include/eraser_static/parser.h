#include <clang-c/Index.h>
#include <string>
#include <iostream>

class Parser {
    public:
        const std::string filepath;
        CXCursor ast;
        Parser(std::string path);

        void Parse();

        void dump_AST();
    private:


};




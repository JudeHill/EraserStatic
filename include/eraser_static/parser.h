#include <clang-c/Index.h>
#include <string>
#include <iostream>

class SmallParser {
    public:
        const std::string filepath;
        CXCursor ast;
        SmallParser(std::string path);

        void Parse();

        void dump_AST();
    private:


};




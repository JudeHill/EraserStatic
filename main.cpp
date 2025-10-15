// to be implemented
#include <iostream>
#include "include/eraser_static/parser.h"

int main(int argc, char* argv[]){
    if (argc != 2){
        std::cerr << "Incorrect number of arguments. Expected usage: eraser-static <filename>" << std::endl;
        exit(-1);
    }
    std::string filename(argv[1]);
    Parser parser(filename);
    parser.Parse();
    parser.dump_AST();
    return 0;

}
// to be implemented
#include <iostream>
#include "parser.h"
#include "graph_visualizer.h"

int main(int argc, char* argv[]){
    if (argc != 2){
        std::cerr << "Incorrect number of arguments. Expected usage: eraser-static <filename>" << std::endl;
        exit(-1);
    }
    std::string filename(argv[1]);
    CallGraph *cg = new CallGraph();
    FileIncludes *fi = new FileIncludes();
    Parser parser(cg, fi);
    
    parser.parseFile(filename.c_str(), true);
   
    parser.visualizeCFG();
    delete cg;
    delete fi;
    return 0;

}
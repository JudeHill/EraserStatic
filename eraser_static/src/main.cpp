// to be implemented
#include <iostream>
#include "parser.h"
#include "graph_visualizer.h"
#include "lockset.h"
#include <system_error>

void dump_data_races(std::string filepath, std::vector<DataRace> data_races){
    auto out_stream = std::ofstream(filepath);
    if (!out_stream) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "failed to open output file: " + filepath
        );
    }
    for (const auto& dr : data_races){
        out_stream << dr.var_name << dr.node->getPrintableNameWithId() << "\n";
    }

}

int main(int argc, char* argv[]){
    if (argc < 3){
        std::cerr << "Incorrect number of arguments. Expected usage: eraser-static <in_filename> <out_filename> <options>" << std::endl;
        exit(-1);
    }
    std::string filename(argv[1]);
    std::string out_filename(argv[2]);
    bool debug = false;
    bool show_graph = false;
    int i = 3;
    while (i < argc){
        std::string option = std::string(argv[i]);
        if (option == "-v" || option == "--verbose"){
            debug = true;
        } else if (option == "--show-graph" || option == "-g"){
            show_graph = true;
        } else {
            std::cerr << "Unknown option: " << option << "\n";

        }

    }
    auto cg = std::make_unique<CallGraph>();
    auto fi = std::make_unique<FileIncludes>();
    Parser parser(cg.get(), fi.get());
    
    parser.parseFile(filename.c_str(), true);
    FuncNodeMap func_cfgs = parser.getFunctionCfgs();
    if (show_graph){
        parser.visualizeCFG();
    }
    Eraser eraser = Eraser();
    std::vector<DataRace> data_races = eraser.compute_data_races(func_cfgs, "main", debug);
    dump_data_races(out_filename, data_races);
    return 0;

}


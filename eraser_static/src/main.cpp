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
    std::cout << "Started" << std::endl;
    std::string filename(argv[1]);
    std::string out_filename(argv[2]);
    bool debug = false;
    bool show_graph = false;
    bool is_barnes = false;
    int i = 3;
    while (i < argc){
        std::string option = std::string(argv[i]);
        if (option == "-v" || option == "--verbose"){
            debug = true;
        } else if (option == "--show-graph" || option == "-g"){
            show_graph = true;
        } else if (option == "-b" || option == "--barnes"){
            is_barnes = true;
        } else {
            std::cerr << "Unknown option: " << option << "\n";

        }
        i++;

    }
    auto cg = std::make_unique<CallGraph>();
    auto fi = std::make_unique<FileIncludes>();
    std::cout << "started parsing" << std::endl;
    Parser parser(cg.get(), fi.get());
    if (is_barnes){
        std::cout << "Parsing barnes" << std::endl;
        std::vector<std::string> filenames{"grav.c", "load.c", "util.c", "code.c", "getparam.c"};
        for (auto& name : filenames){
            parser.parseFile(name.c_str(), true);
        }
    } else {
         parser.parseFile(filename.c_str(), true);
    // parser.parseFile("extern.c", true);
    }

   
    std::cout << "parsed" << std::endl;
    FuncNodeMap func_cfgs = parser.getFunctionCfgs();
    std::cout << "Got func cfgs" << std::endl;
    if (show_graph){
        std::cout << "Started CFG visualise" << std::endl;
        parser.visualizeCFG();
        std::cout << "Ended" << std::endl;
    }
    Eraser eraser = Eraser();
    std::cout << "Computing data races" << std::endl;
    std::vector<DataRace> data_races = eraser.compute_data_races(func_cfgs, "main", debug);
    dump_data_races(out_filename, data_races);
    std::cout << "Finished" << std::endl;
    
    return 0;

}


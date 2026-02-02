// to be implemented
#include <iostream>
#include "parser.h"
#include "graph_visualizer.h"
#include "lockset.h"
#include <system_error>
#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

void dump_data_races(std::string filepath, DataRaceMap data_race_map){
    auto out_stream = std::ofstream(filepath);
    if (!out_stream) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "failed to open output file: " + filepath
        );
    }
    out_stream << std::format("Found dataraces involving {} unique variables", data_race_map.size()) << std::endl;
    for (const auto& [var_name, data_races] : data_race_map){
        out_stream << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
        for (int i=0;i<std::min<std::size_t>(5, data_races.size());i++){
            DataRace dr = data_races[i];
            if (dr.race_type == RACE_READ){
                out_stream << "Read";
            } else {
                out_stream << "Write";
            }
            out_stream << " in file " << dr.location.file_name << " at line " << dr.location.line << ", position " << dr.location.column << "\n";
        }
        if (data_races.size() > 5){
            out_stream << "..." << "\n";
        }
        
    }

}

int main(int argc, char* argv[]){
    
    if (argc < 3){
        std::cerr << "Incorrect number of arguments. Expected usage: eraser-static <in_filename> <out_filename> <options>" << std::endl;
        exit(-1);
    }

    std::cout << "Started" << std::endl;
    std::string filepath(argv[1]);
    bool directory_mode = !filepath.ends_with(".c");
    std::string out_filename(argv[2]);
    bool debug = false;
    bool show_graph = false;
    bool is_barnes = false;
    bool symmetric_join = false;
    int i = 3;
    while (i < argc){
        std::string option = std::string(argv[i]);
        if (option == "-v" || option == "--verbose"){
            debug = true;
        } else if (option == "--show-graph" || option == "-g"){
            show_graph = true;
        } else if (option == "-s" || option == "--symmetric-join"){
            symmetric_join = true;
        } else {
            std::cerr << "Unknown option: " << option << "\n";

        }
        i++;

    }
    std::vector<fs::path> files;
    auto cg = std::make_unique<CallGraph>();
    auto fi = std::make_unique<FileIncludes>();
    std::cout << "started parsing" << std::endl;
    Parser parser(cg.get(), fi.get());
   
    if (!directory_mode){
        parser.parseFile(filepath.c_str(), true);
    } else {
        try {
            if (fs::exists(filepath) && fs::is_directory(filepath)) {
                for (const auto& entry : fs::directory_iterator(filepath)) {
                    // Check if it's a regular file and ends with .c
                    if (entry.is_regular_file() && entry.path().extension() == ".c") {
                        files.push_back(entry.path());
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }  
    }
         
    for (const auto& filePath : files) {
        // 2. Convert path to a C-string
        // .string() creates a std::string; .c_str() gets the const char*
        std::string pathStr = filePath.string();
        const char* fileName = pathStr.c_str();

        std::cout << "Parsing: " << fileName << "..." << std::endl;
        parser.parseFile(fileName);
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
    DataRaceMap data_race_map = eraser.compute_data_races(func_cfgs, "main", debug, symmetric_join);
    dump_data_races(out_filename, data_race_map);
    std::cout << "Finished" << std::endl;
    
    return 0;

}


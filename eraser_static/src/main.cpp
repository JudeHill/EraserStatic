// to be implemented
#include "graph_visualizer.h"
#include "llm_handler.h"
#include "eval_llms.h"
#include "parser.h"
#include "shared_var_identifier.h"
#include <iostream>
#include "llm_analyser.h"

#include "lockset.h"
#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>
#include <CLI/CLI.hpp>

namespace fs = std::filesystem;

struct Options {
  std::string input_path;
  std::string output_path;

  bool debug = false;
  bool show_graph = false;
  bool is_barnes = false;
  bool symmetric_join = false;
  bool ignore_barriers = false;
  bool slow_llm_requests = false;
  bool evaluating_llms = false;
  bool test_llms = false;
  bool write_all_races = false;
};

struct Results {
  DataRaceMap data_race_map;
  SharedVarResults shvar_results;
  FalsePosResults false_pos_results;
};

void write_output(std::string filepath, Results results, bool write_all_races = false) {
  auto out_stream = std::ofstream(filepath);
  if (!out_stream) {
    throw std::system_error(errno, std::generic_category(),
                            "failed to open output file: " + filepath);
  }
  out_stream << std::format("Found dataraces involving {} unique variables", results.data_race_map.size())
             << std::endl;
  
  for (const auto &[var_name, data_races] : results.data_race_map) {
    int num_races_to_write = write_all_races ? data_races.size() : std::min<std::size_t>(ACCESSES_PER_VAR, data_races.size());
    out_stream << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
    for (int i = 0; i < num_races_to_write; i++) {
      DataRace dr = data_races[i];
      if (dr.race_type == RACE_READ) {
        out_stream << "Read";
      } else {
        out_stream << "Write";
      }
      out_stream << " in file " << dr.location.file_name << " at line " << dr.location.line
                 << ", position " << dr.location.column << "\n";
    }
    if (data_races.size() > num_races_to_write) {
      out_stream << "..." << "\n";
    }
  }
  out_stream << "\n";
  out_stream << "LLM analysis of shared variables:" << "\n";
  SharedVarIdentifier shvar_id;
  for (const auto &[llm, result] : results.shvar_results) {
    out_stream << get_llm_name(llm) << "\n";
    out_stream << "TP: " << result.true_pos << "   FP: " << result.false_pos
               << "   FN: " << result.false_neg << "\n";
  }
  out_stream << "LLM analysis of false positives of data races: " << "\n";
  for (const auto& llm : all_llms){
    out_stream << get_llm_name(llm) << "\n";
    for (const VarResult& var_result : results.false_pos_results[llm]){
      out_stream << "Variable " << var_result.var_name << "\n";
      out_stream << (var_result.has_data_race ? "Has a data race" : "No data race") << "\n";
      out_stream << var_result.true_pos_accesses.size() << " true unprotected accesses, ";
      out_stream << var_result.false_pos_accesses.size() << " false positives" << "\n";
      out_stream << "False positives: " << "\n";
      for (const LLM_DataRace& llm_data_race : var_result.false_pos_accesses){
        out_stream << (llm_data_race.data_race.race_type == RACE_WRITE ? "Write " : "Read ");
        out_stream << "in file " << llm_data_race.data_race.location.file_name << ", ";
        out_stream << "on line " << llm_data_race.data_race.location.line << " ";
        out_stream << "at position " << llm_data_race.data_race.location.column << "\n";
        out_stream << "Reasoning: " << llm_data_race.reasoning << "\n";
      } 
      out_stream << "True positives: " << "\n";
      for (const LLM_DataRace& llm_data_race : var_result.true_pos_accesses){
        out_stream << (llm_data_race.data_race.race_type == RACE_WRITE ? "Write " : "Read ");
        out_stream << "in file " << llm_data_race.data_race.location.file_name << ", ";
        out_stream << "on line " << llm_data_race.data_race.location.line << " ";
        out_stream << "at position " << llm_data_race.data_race.location.column << "\n";
        out_stream << "Reasoning: " << llm_data_race.reasoning << "\n";
      }  
    }
  }
}

int main(int argc, char *argv[]) {

  std::cout << "Started" << std::endl;

  Options opts;

  CLI::App app{"eraser-static"};

  // Positional arguments
  app.add_option("in_filename", opts.input_path, 
                 "Input .c file or directory")->required();

  app.add_option("out_filename", opts.output_path, 
                 "Output filename")->required();

  // Flags
  app.add_flag("-v,--verbose", opts.debug, "Verbose output");
  app.add_flag("-g,--show-graph", opts.show_graph, "Show graph");
  app.add_flag("-s,--symmetric-join", opts.symmetric_join, "Use symmetric join");
  app.add_flag("-b,--no-barrier", opts.ignore_barriers, "Ignore barriers");
  app.add_flag("--eval-llms", opts.evaluating_llms, "Evaluate LLMs");
  app.add_flag("--slow-llms", opts.slow_llm_requests, "Slow down LLMs to avoid rate limiting");
  app.add_flag("--test-llms", opts.test_llms, "Test LLM connectivity");
  app.add_flag("--write-all", opts.write_all_races, "Write all reported unprotected accesses to out, instead of just the first 5 per variable");

  CLI11_PARSE(app, argc, argv);

  bool directory_mode = !opts.input_path.ends_with(".c");

  if (opts.test_llms) {
      test_llms_alive();
      return 0;
  }

  if (opts.evaluating_llms){
    eval_shared_variable_llm_consistency(opts.input_path, opts.slow_llm_requests);
    return 0;
  }
  std::vector<fs::path> files;
  auto cg = std::make_unique<CallGraph>();
  auto fi = std::make_unique<FileIncludes>();
  std::cout << "started parsing" << std::endl;
  Parser parser(cg.get(), fi.get());

  if (!directory_mode) {
    parser.parseFile(opts.input_path.c_str(), opts.ignore_barriers);
  } else {
    try {
      if (fs::exists(opts.input_path) && fs::is_directory(opts.input_path)) {
        for (const auto &entry : fs::directory_iterator(opts.input_path)) {
          // Check if it's a regular file and ends with .c
          if (entry.is_regular_file() &&
              (entry.path().extension() == ".c" || entry.path().extension() == ".C")) {
            files.push_back(entry.path());
          }
        }
      }
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  for (const auto &filePath : files) {
    // 2. Convert path to a C-string
    // .string() creates a std::string; .c_str() gets the const char*
    std::string pathStr = filePath.string();
    const char *fileName = pathStr.c_str();

    std::cout << "Parsing: " << fileName << "..." << std::endl;
    parser.parseFile(fileName, opts.ignore_barriers);
  }

  std::cout << "parsed" << std::endl;
  FuncNodeMap func_cfgs = parser.getFunctionCfgs();
  std::cout << "Got func cfgs" << std::endl;
  if (opts.show_graph) {
    std::cout << "Started CFG visualise" << std::endl;
    parser.visualizeCFG();
    std::cout << "Ended" << std::endl;
  }
  Eraser eraser = Eraser();
  std::cout << "Computing data races" << std::endl;
  DataRaceMap data_race_map = eraser.compute_data_races(func_cfgs, "main", opts.debug, opts.symmetric_join);

  std::cout << "Starting LLM analysis" << std::endl;
  SharedVarIdentifier shared_var_id;
  SharedVarInfos shared_vars_llm_info = shared_var_id.findSharedVariables(opts.input_path);
  SharedVarResults shvar_results = shared_var_id.EvaluateLLMs(shared_vars_llm_info);
  LLMAnalyser llm_analyser(opts.input_path);
  FalsePosResults false_pos_results = llm_analyser.ParseResults(llm_analyser.FilterFalsePositives(data_race_map));

  std::cout << "Writing output" << std::endl;
  write_output(
    opts.output_path,
    Results{
      .data_race_map = data_race_map,
      .shvar_results = shvar_results,
      .false_pos_results = false_pos_results,
    }, 
    opts.write_all_races);
  std::cout << "Finished" << std::endl;

  return 0;
}

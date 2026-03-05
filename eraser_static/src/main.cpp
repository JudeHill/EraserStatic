// to be implemented
#include "graph_visualizer.h"
#include "llm_handler.h"
#include "eval_llms.h"
#include "parser.h"
#include "shared_var_identifier.h"
#include "write_output.h"
#include <iostream>
#include "llm_analyser.h"

#include "lockset.h"
#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>
#include <CLI/CLI.hpp>

namespace fs = std::filesystem;



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
  app.add_flag("--eval-fps", opts.eval_llms_fps, "Evaluate false positives using LLMs");
  app.add_flag("--slow-llms", opts.slow_llm_requests, "Slow down LLMs to avoid rate limiting");
  app.add_flag("--test-llms", opts.test_llms, "Test LLM connectivity");
  app.add_flag("--write-all", opts.write_all_races, "Write all reported unprotected accesses to out, instead of just the first 5 per variable");
  app.add_flag("-t, --variant-responses", opts.variant_llm_responses, "Turn LLM temperature up (0.2) to allow non-deterministic responses");

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
  if (opts.eval_llms_fps){
    SummaryFalsePosResults summary_fp_results = llm_analyser.EvalFalsePosLLMConsistency(data_race_map, shvar_results, opts.slow_llm_requests);

    std::cout << "Writing output" << std::endl;
    write_fp_eval_output(
      opts.output_path,
      EvalLLMResults{
        .data_race_map = data_race_map,
        .shvar_results = shvar_results,
        .false_pos_results = summary_fp_results,
      }, 
      opts.write_all_races);
  } else {
    FalsePosResults fp_results = llm_analyser.ParseResults(llm_analyser.FilterFalsePositives(data_race_map, shvar_results));

    std::cout << "Writing output" << std::endl;
    write_output(
      opts.output_path,
      Results{
        .data_race_map = data_race_map,
        .shvar_results = shvar_results,
        .false_pos_results = fp_results,
      }, 
      opts.write_all_races);
  }
  
  std::cout << "Finished" << std::endl;

  return 0;
}

#pragma once
#include "lockset.h"
#include "shared_var_identifier.h"
#include "llm_analyser.h"
#define ACCESSES_PER_VAR 5

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
    bool no_llms = false;
    bool eval_llms_fps = false;
    bool eval_llms_fns = false;
    bool test_llms = false;
    bool write_all_races = false;
    bool variant_llm_responses = false;
  };
  
  struct Results {
    DataRaceMap data_race_map;
    SharedVarResults shvar_results;
    FalsePosResults false_pos_results;
    FalseNegResults false_neg_results;
  };

  struct EvalLLMResults {
    DataRaceMap data_race_map;
    SharedVarResults shvar_results;
    SummaryFalsePosResults false_pos_results;
    SummaryFalseNegResults false_neg_results;
  };

  void write_output(const Filepath& filepath, const Results& results, bool write_all_races = false);
  void write_output(const Filepath& filepath, const DataRaceMap& data_race_map, bool write_all_races = true);
  void write_fp_eval_output(const Filepath& filepath, EvalLLMResults results, bool write_all_races = true);
  void write_fn_eval_output(const Filepath& filepath, EvalLLMResults results, bool write_all_races = true);
  
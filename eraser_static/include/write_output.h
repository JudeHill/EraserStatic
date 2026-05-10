/* 
 * Project: EraserStatic
 * (https://github.com/JudeHill/EraserStatic)
 *
 * Copyright (C) 2025-2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once
#include "lockset.h"
#include "shared_var_identifier.h"
#include "llm_analyser.h"
#define ACCESSES_PER_VAR 5

struct Options {
    std::string input_path;
    std::string output_path;
    std::string llm;
  
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
  
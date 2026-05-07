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
#include "llm_handler.h"
#include "usings.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <iomanip>
#define LLM_API_DELAY_SECONDS 20

using nlohmann::json;
namespace fs = std::filesystem;
using SharedVarInfos = std::unordered_map<LLM, json>;
using VoteSet = std::unordered_set<std::string>;

struct LLM_result {
  VoteSet votes;
  unsigned int true_pos, false_pos, false_neg;
};

struct LLM_SummaryResult {
  double jaccard_score;
  double avg_tp, avg_fp, avg_fn;
  std::vector<LLM_result> results;
  std::unordered_map<std::string, unsigned int> var_vote_counts;
};
using SummaryResults = std::unordered_map<LLM, LLM_SummaryResult>;
using SharedVarResults = std::unordered_map<LLM, LLM_result>;
void create_prompt(std::ostringstream &oss, const Filepath &filepath, const std::string &prompt, bool number_lines = false);

class SharedVarIdentifier {
public:
  SharedVarIdentifier();
  SharedVarInfos findSharedVariables(const Filepath &filepath);
  SharedVarResults EvaluateLLMs(const SharedVarInfos &infos);
  SummaryResults EvaluateLLMConsistency(const Filepath &filepath, bool slow_llm_requests,
                                        unsigned int repeats = 5);

private:
  LLMHandler llm_handler;
};
#pragma once
#include "llm_handler.h"
#include "usings.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using nlohmann::json;
namespace fs = std::filesystem;
using SharedVarInfos = std::unordered_map<LLM, json>;
using VoteSet = std::unordered_set<std::string>;

struct LLM_result {
  VoteSet votes;
  unsigned int true_pos, false_pos, false_neg;
};

struct LLM_SummaryResult {
    double jacquard_score;
    unsigned int avg_tp, avg_fp, avg_fn;
    std::vector<LLM_result> results;
};
using SummaryResults = std::unordered_map<LLM, LLM_SummaryResult>;
using SharedVarResults = std::unordered_map<LLM, LLM_result>;

class SharedVarIdentifier {
public:
  SharedVarIdentifier();
  SharedVarInfos findSharedVariables(const Filepath &filepath);
  SharedVarResults EvaluateLLMs(const SharedVarInfos &infos);
  SummaryResults EvaluateLLMConsistency(const SharedVarInfos &infos, unsigned int repeats = 5);

private: 
    LLMHandler llm_handler;
};
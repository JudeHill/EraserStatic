#pragma once
#include "llm_handler.h"
#include "usings.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <thread>
#include <format>
#include <fstream>
#include <future>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
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
    double jacquard_score;
    double avg_tp, avg_fp, avg_fn;
    std::vector<LLM_result> results;
    std::unordered_map<std::string, unsigned int> var_vote_counts;
};
using SummaryResults = std::unordered_map<LLM, LLM_SummaryResult>;
using SharedVarResults = std::unordered_map<LLM, LLM_result>;
void create_prompt(std::ostringstream& oss, const Filepath& filepath, const std::string& prompt);

class SharedVarIdentifier {
public:
  SharedVarIdentifier();
  SharedVarInfos findSharedVariables(const Filepath &filepath);
  SharedVarResults EvaluateLLMs(const SharedVarInfos &infos);
  SummaryResults EvaluateLLMConsistency(const Filepath &filepath, bool slow_llm_requests, unsigned int repeats = 5);

private: 
    LLMHandler llm_handler;
};
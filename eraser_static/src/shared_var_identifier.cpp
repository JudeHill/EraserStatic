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

#include "shared_var_identifier.h"

std::string parse_c_file(const std::string &path, bool number_lines = false) {
  std::ifstream file(path);
  if (!file.is_open())
      throw std::logic_error("Couldn't open file");

  std::stringstream result;
  std::string line;
  int line_count = 1;

  while (std::getline(file, line)) {
      if (number_lines) {
          // Right-align numbers for a clean "IDE-like" look
          result << std::setw(4) << line_count << " | " << line << "\n";
          line_count++;
      } else {
          result << line << "\n";
      }
  }

  return result.str();
}

void create_prompt(std::ostringstream& oss, const Filepath& filepath, const std::string& prompt, bool number_lines){
  bool directory_mode = !filepath.ends_with(".c");
  oss << prompt << "\n";
  oss << "SOURCE CODE:" << "\n";
  oss << "---" << "\n";
  if (!directory_mode) {
    oss << filepath << "\n";
    oss << parse_c_file(filepath, number_lines);
  } else {
    std::vector<fs::path> files;
    try {
      if (fs::exists(filepath) && fs::is_directory(filepath)) {
        for (const auto &entry : fs::directory_iterator(filepath)) {
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
    for (const auto &file : files) {
      oss << file.c_str() <<"\n";
      oss << parse_c_file(file.string(), number_lines);
    }
  }
}

static const std::string shared_var_prompt = R"(### Role
You are a static analysis tool specializing in C concurrency. Your task is to identify shared variables in a given code snippet.

### Definition
A variable is "SHARED" if:
1. It is declared in a scope accessible by multiple threads (e.g., global, static, or passed by reference/pointer to a thread).
    - Note specifically, global static variables (i.e. file-local) are **SHARED** in this context
2. It is a heap-allocated object accessed by multiple thread contexts.



### Constraints
- Identify only the variables, and their types.
- Fields of a pointer are **NOT** distinct shared variables: only the pointer itself is a shared variable.
- Variable names should be identified by the name they would be accessed with in the program. For example, if 
    a variable is declared as static float *A, its name is A, and its type is static float *
- Variable types should be identified by the type the variable is declared as. For example, if an array is declared
 as static float *A, its type is static float *, not static float[]. 
- Array types should not include sizes: e.g. int x[5] has type int[]
- the "name" and "type" fields of the output should contain **ONLY** the name and type as defined above, without including **ANYTHING ELSE**
- Return ONLY the JSON object. Do not include markdown code blocks (like ```json), explanations, or conversational filler.
- Comments should be **EMPTY** or **INCREDIBLY CONCISE**. Do not include definitions, descriptions of shared variables etc. Only comment if there was an edge case worth mentioning.
- Ensure all braces are escaped correctly if processed via a template engine.

### Output Format
{{
  "variables": [
    {{ "name": "variable_name", "type": "data_type" }}
  ],
  "comments": "Brief explanation of why these were identified as shared."
}}

### Input Code
)";

SharedVarInfos SharedVarIdentifier::findSharedVariables(const Filepath &filepath) {
  std::ostringstream oss;
  create_prompt(oss, filepath, shared_var_prompt);
  std::string_view prompt = oss.view();
  json schema = {
      {"type", "object"},
      {"properties",
       {{"variables",
         {{"type", "array"},
          {"items",
           {{"type", "object"},
            {"properties", {{"name", {{"type", "string"}}}, {"type", {{"type", "string"}}}}},
            {"required", {"name", "type"}},
            {"additionalProperties", false}}}}},
        {"comments", {{"type", "string"}}}}},
      {"required", {"variables", "comments"}},
      {"additionalProperties", false}};
  SharedVarInfos shvar_infos;
  std::unordered_map<LLM, std::future<json>> futures;
  for (const LLM llm : all_llms){
    auto task = std::async(std::launch::async, [this, prompt, schema, llm]{
      return this->llm_handler.PromptWithRetries(prompt, schema, llm);
    });
    futures[llm] = std::move(task);
  }
  for (const LLM llm : all_llms){
    shvar_infos[llm] = futures[llm].get();
  }

  return shvar_infos;
}

SharedVarResults SharedVarIdentifier::EvaluateLLMs(const SharedVarInfos &infos) {
  // majority voting
  bool disagreement = false;
  SharedVarResults results;
  struct VoteInfo {
    std::string var_name;
    std::unordered_set<LLM> votes;
  };
  std::unordered_map<std::string, VoteInfo> votes;
  for (const auto &[llm, response] : infos) {
    // variables are stored as json with "name" and "type" fields. We
    // only care here about names
    for (const auto &var : response.at("variables").get<std::vector<json>>()) {
      if (!votes.contains(var.at("name"))) {
        votes[var.at("name")] = VoteInfo{
            .var_name = var.at("name"),
            .votes = {},
        };
      }
      votes[var.at("name")].votes.insert(llm);
      results[llm].votes.insert(var.at("name"));
    }
  }
  for (const auto &[name, vote_info] : votes) {
    int num_votes = vote_info.votes.size();
    if (num_votes < 3) {
      std::cout << "Disagreement on variable " << name << std::endl;
      std::cout << "Yes votes: ";
      for (const auto &llm : vote_info.votes) {
        std::cout << get_llm_name(llm) << " ";
      }
      std::cout << std::endl << "No votes:";
      for (const auto &llm : all_llms) {
        if (!vote_info.votes.contains(llm))
          std::cout << get_llm_name(llm) << " ";
      }
      std::cout << std::endl;
      disagreement = true;
    }
    if (num_votes >= 2) {
      // majority
      for (LLM llm : all_llms) {
        if (vote_info.votes.contains(llm)) {
          results[llm].true_pos++;
        } else {
          results[llm].false_neg++;
        }
      }
    } else {
      for (LLM llm : vote_info.votes) {
        results[llm].false_pos++;
      }
    }
  }
  if (disagreement) {
    std::cout << "Disagreement - printing outputs" << std::endl;
    for (const auto &[llm, response] : infos) {
      std::cout << get_llm_name(llm) << std::endl;
      std::cout << response.dump(4) << std::endl;
    }
  }
  return results;
}

void worker(int tid, std::vector<SharedVarResults> results, SharedVarIdentifier &id, const Filepath& filepath) {
    SharedVarInfos infos = id.findSharedVariables(filepath);
    results[tid] = id.EvaluateLLMs(infos);
}

SummaryResults SharedVarIdentifier::EvaluateLLMConsistency(const Filepath &filepath, bool slow_llm_requests, unsigned int repeats){
    std::vector<SharedVarResults> results;
    SummaryResults summary_results;
    std::vector<std::future<SharedVarResults>> futures;
    
    for (unsigned int i=0;i<repeats;i++){
        futures.push_back(std::async(std::launch::async, [this, &filepath](){
            return EvaluateLLMs(findSharedVariables(filepath));
        }));
        // do requests one at a time to avoid rate limiting
        if (slow_llm_requests){
          results.push_back(futures.back().get());
          std::this_thread::sleep_for(std::chrono::seconds(LLM_API_DELAY_SECONDS));
        } 
    }
    // collect results if we have not already done so
    if (!slow_llm_requests){
      for (auto& f : futures){
        results.push_back(f.get());
      }
    }
  
    
    for (const LLM llm : all_llms){
        double jaccard_score = 0.0;
        unsigned int num_jac_sets = 0;
        int total_tp = 0, total_fp = 0, total_fn = 0;
        std::vector<LLM_result> individual_results_list;
        std::unordered_map<std::string, unsigned int> var_vote_counts;
        for (int i=0;i<repeats;i++){
            for (int j=i+1;j<repeats;j++){
                VoteSet s1_union_s2 = set_union(results[i][llm].votes, results[j][llm].votes);
                VoteSet s1_intersect_s2 = intersect(results[i][llm].votes, results[j][llm].votes);
                jaccard_score += (static_cast<double>(s1_intersect_s2.size()) / s1_union_s2.size());
                num_jac_sets++;
            }
            for (const auto& var : results[i][llm].votes) {
                var_vote_counts[var]++;
            }
            total_tp += results[i][llm].true_pos;
            total_fp += results[i][llm].false_pos;
            total_fn += results[i][llm].false_neg;
            individual_results_list.push_back(results[i][llm]);
        }
        summary_results[llm] = LLM_SummaryResult{
            .jaccard_score = (jaccard_score / num_jac_sets),
            .avg_tp = (static_cast<double>(total_tp) / repeats),
            .avg_fp = (static_cast<double>(total_fp) / repeats),
            .avg_fn = (static_cast<double>(total_fn) / repeats),
            .results = individual_results_list,
            .var_vote_counts = var_vote_counts,
        };
    }
    return summary_results;
    

}

SharedVarIdentifier::SharedVarIdentifier() {};
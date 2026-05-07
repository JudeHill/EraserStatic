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


#include "eval_llms.h"

void test_llms_alive() {
  LLMHandler handler;
  std::string prompt = "Confirm what LLM you are, and what version I am talking to";
  std::vector llms{LLM::CLAUDE, LLM::GEMINI, LLM::GPT};
  json schema = {{"type", "object"},
                 {"properties", {{"response", {{"type", "string"}}}}},
                 {"required", {"response"}},
                 {"additionalProperties", false}};
  for (auto llm : llms) {
    json j = handler.Prompt(prompt, schema, llm);
    std::cout << j.dump(4) << std::endl;
  }
}

void eval_shared_variable_llm_consistency(const Filepath &in_filepath, bool slow_llm_requests,
                                          const Filepath &out_filepath) {
  SharedVarIdentifier identifier;
  SummaryResults result = identifier.EvaluateLLMConsistency(in_filepath, slow_llm_requests);
  auto out_stream = std::ofstream(out_filepath);
  if (!out_stream) {
    throw std::system_error(errno, std::generic_category(),
                            "failed to open output file: " + out_filepath);
  }
  for (const LLM llm : all_llms) {
    out_stream << "Results for " << get_llm_name(llm) << "\n";
    out_stream << "Average TP: " << result[llm].avg_tp << "  ";
    out_stream << "Average FP: " << result[llm].avg_fp << "  ";
    out_stream << "Average FN: " << result[llm].avg_fn << "\n";
    out_stream << "Jacquard pairwise agreement between iterations (5 iterations): "
               << result[llm].jaccard_score << "\n";
    out_stream << "Vote counts: " << "\n";
    for (const auto &[var, votes] : result[llm].var_vote_counts) {
      out_stream << var << ": " << votes << "\n";
    }
  }
}

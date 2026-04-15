#pragma once
#include "llm_handler.h"
#include "lockset.h"
#include "shared_var_identifier.h"
#include "string"
#include "usings.h"
#define ACCESSES_PER_VAR 5

enum class RaceVerdict {
  TRUE_POS,
  FALSE_POS,
  FALSE_NEG,
};

struct LLM_DataRace {
  DataRace data_race;
  bool true_pos;
  std::string reasoning;
};

struct VarResult {
  VarName var_name;
  bool has_data_race;
  std::unordered_map<unsigned int, LLM_DataRace> false_pos_accesses;
  std::unordered_map<unsigned int, LLM_DataRace> true_pos_accesses;
};

struct FalseNegVarResult {
  VarName var_name;
  std::vector<LLM_DataRace> unprotected_accesses;
};

using FalseNegResult = std::vector<FalseNegVarResult>;
using FalseNegResults = std::unordered_map<LLM, FalseNegResult>;

/*
What do i want here?
Need no notion of TP/FP because too unstable, but need Fleiss Kappa (single number)
Need this PER LLM.
Need the list of data races, their IDs, and the votes for each one.
*/

struct FalsePosRunInfo {
  std::unordered_set<DataRaceID> access_votes_tp, access_votes_fp;
  std::unordered_set<VarName> var_votes_tp, var_votes_fp;
};

using FalsePosRunInfos = std::unordered_map<LLM, std::vector<FalsePosRunInfo>>;

struct FalseNegRunInfo {
  // Accesses are keyed by a string concatenation of line, var name and access type (R/W). This is because LLMs cannot identify the 
  // column of an access reliably. Because of this, we use a map, since we can have multiple races with the same key
  std::unordered_map<std::string, unsigned int> access_votes;
  std::unordered_set<VarName> var_votes;
};

using FalseNegRunInfos = std::unordered_map<LLM, std::vector<FalseNegRunInfo>>;

struct JaccardAgreement {
  double var_agreement, access_agreement;
};

struct SummaryFalsePosResult {
  JaccardAgreement jaccard_agreement;
  // maps DataRaceId -> vote count (this is already mapped per LLM)
  std::unordered_map<DataRaceID, unsigned int> tp_votes_accesses, fp_votes_accesses;
  // maps VarName (string) -> vote count
  std::unordered_map<VarName, unsigned int> tp_votes_variables, fp_votes_variables;
};

struct SummaryFalsePosResults {
  std::unordered_map<LLM, SummaryFalsePosResult> results;
  DataRaceMap data_race_map;
};

// need: votes per variable and per access

struct FalseNegAccess {
  LLM_DataRace access;
  unsigned int votes;
};

struct FalseNegVariable {
  std::unordered_map<std::string, FalseNegAccess> accesses;
  unsigned int votes;
};

struct SummaryFalseNegResult {
  JaccardAgreement jaccard_agreement;
  std::unordered_map<VarName, FalseNegVariable> variables;
};

struct SummaryFalseNegResults {
  std::unordered_map<LLM, SummaryFalseNegResult> results;
  DataRaceMap data_race_map;
};

std::string get_datarace_key(const DataRace& data_race);

using FalsePosResult = std::vector<VarResult>;
using FalsePosResults = std::unordered_map<LLM, FalsePosResult>;
using JsonResults = std::unordered_map<LLM, std::vector<json>>;

class LLMAnalyser {
public:
  JsonResults FilterFalsePositives(const DataRaceMap &data_race_map,
                                   const SharedVarResults &shvar_results);
  void HintsToProgrammer();
  void TestFilterFalsePositives(const DataRaceMap &data_race_map,
                                const SharedVarResults &shvar_results);
  LLMAnalyser(const Filepath filepath);
  FalsePosResults ParseFalsePosResults(const JsonResults &json_results);
  JsonResults FindFalseNegatives(const DataRaceMap &data_race_map,
                                 const SharedVarResults &shvar_results);
  FalseNegResults ParseFalseNegResults(const JsonResults &json_results);
  SummaryFalsePosResults EvalFalsePosLLMConsistency(const DataRaceMap &data_race_map,
                                                    const SharedVarResults &shvar_results,
                                                    bool slow_llms = false,
                                                    unsigned int repeats = 5U);
  SummaryFalseNegResults EvalFalseNegLLMConsistency(const DataRaceMap &data_race_map,
                                                    const SharedVarResults &shvar_results,
                                                    bool slow_llms = false,
                                                    unsigned int repeats = 5U);

private:
  const Filepath filepath;
  LLMHandler llm_handler;
  SharedVarIdentifier shared_var_identifier;
};
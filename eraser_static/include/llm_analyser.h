#pragma once
#include "shared_var_identifier.h"
#include "llm_handler.h"
#include "string"
#include "lockset.h"
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

struct FalsePosLLMAgreement {
    double var_agreement, access_agreement;
};


struct SummaryFalsePosResult {
    FalsePosLLMAgreement jaccard_agreement;
    // maps DataRaceId -> vote count (this is already mapped per LLM)
    std::unordered_map<DataRaceID, unsigned int> tp_votes_accesses, fp_votes_accesses;
    // maps VarName (string) -> vote count
    std::unordered_map<VarName, unsigned int> tp_votes_variables, fp_votes_variables;
};

struct SummaryFalsePosResults {
    std::unordered_map<LLM, SummaryFalsePosResult> results;
    DataRaceMap data_race_map;
};
using FalsePosResult = std::vector<VarResult>;
using FalsePosResults = std::unordered_map<LLM, FalsePosResult>;
using JsonResults = std::unordered_map<LLM, std::vector<json>>;

class LLMAnalyser {
    public:
        JsonResults FilterFalsePositives(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results);
        void HintsToProgrammer();
        void TestFilterFalsePositives(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results);
        LLMAnalyser(const Filepath filepath);
        FalsePosResults ParseResults(const JsonResults& json_results);
        SummaryFalsePosResults EvalFalsePosLLMConsistency(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results, bool slow_llms = false, unsigned int repeats = 5U);
    private:
        const Filepath filepath;
        LLMHandler llm_handler;
        SharedVarIdentifier shared_var_identifier;

};
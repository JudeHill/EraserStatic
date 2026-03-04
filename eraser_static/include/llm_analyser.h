#pragma once
#include "shared_var_identifier.h"
#include "llm_handler.h"
#include "string"
#include "lockset.h"
#include "usings.h"
#define ACCESSES_PER_VAR 5


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

struct SummaryFalsePosResult {
    double fleiss_kappa;
    // maps DataRaceId -> vote count
    std::unordered_map<unsigned int, unsigned int> votes;
};

using SummaryFalsePosResults = std::unordered_map<LLM, SummaryFalsePosResult>;
using FalsePosResult = std::vector<VarResult>;
using FalsePosResults = std::unordered_map<LLM, FalsePosResult>;
using JsonResults = std::unordered_map<LLM, std::vector<json>>;

class LLMAnalyser {
    public:
        JsonResults FilterFalsePositives(DataRaceMap data_race_map);
        void HintsToProgrammer();
        void TestFilterFalsePositives(DataRaceMap data_race_map);
        LLMAnalyser(Filepath filepath);
        FalsePosResults ParseResults(JsonResults json_results);
        SummaryFalsePosResults EvalFalsePosLLMConsistency(DataRaceMap data_race_map, bool slow_llms = false, unsigned int repeats = 5U);
    private:
        const Filepath filepath;
        LLMHandler llm_handler;
        SharedVarIdentifier shared_var_identifier;

};
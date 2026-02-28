#pragma once
#include "shared_var_identifier.h"
#include "llm_handler.h"

class LLMAnalyser {
    public:
    FilterFalsePositives();
    HintsToProgrammer();
    LLMAnalyser(const Filepath &filepath);
    private:
    const Filepath filepath;
    LLMHandler llm_handler;
    SharedVarIdentifier shared_var_identifier;

}
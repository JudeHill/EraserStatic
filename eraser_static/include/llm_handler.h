#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <unordered_set>
#include <chrono>
#include <thread>

using nlohmann::json;

enum class LLM {
    GPT,
    GEMINI,
    CLAUDE
};

static std::vector all_llms{LLM::CLAUDE, LLM::GEMINI, LLM::GPT};
std::string_view get_llm_name(LLM llm);

class LLMHandler{
public:
    LLMHandler();
    json Prompt(const std::string_view prompt, const json& schema, const LLM& llm);
    json PromptWithRetries(const std::string_view prompt, const json& schema, const LLM& llm, unsigned int retries = 3);
private:
    json PromptGPT(const std::string_view prompt, const json& schema);
    json PromptGemini(const std::string_view prompt, const json& schema);
    json PromptClaude(const std::string_view prompt, const json& schema);
};
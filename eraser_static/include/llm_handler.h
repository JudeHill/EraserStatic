#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

enum class LLM {
    GPT,
    GEMINI,
    CLAUDE
};

class LLMHandler{
public:
    LLMHandler();
    json Prompt(std::string prompt, LLM llm);
private:
    json PromptGPT(std::string prompt);
    json PromptGemini(std::string prompt);
    json PromptClaude(std::string prompt);
};
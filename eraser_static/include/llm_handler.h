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
#include <random>
#include <thread>

using nlohmann::json;

enum class LLM {
    GPT,
    GEMINI,
    CLAUDE
};

// This is NOT const. Gets modified by main if user provides a --llm option
inline std::vector all_llms{LLM::CLAUDE, LLM::GEMINI, LLM::GPT};
std::string_view get_llm_name(LLM llm);

class LLMHandler{
public:
    LLMHandler();
    json Prompt(const std::string_view& prompt, const json& schema, const LLM llm, bool allow_variant_responses = true);
    json PromptWithRetries(const std::string_view& prompt, const json& schema, const LLM llm, bool allow_variant_responses = true, 
        unsigned int retries = 3);
private:
    json PromptGPT(const std::string_view& prompt, const json& schema, bool allow_variant_responses);
    json PromptGemini(const std::string_view& prompt, const json& schema, bool allow_variant_responses);
    json PromptClaude(const std::string_view& prompt, const json& schema, bool allow_variant_responses);
};
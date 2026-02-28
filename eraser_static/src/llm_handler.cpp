#include "llm_handler.h"
#define GPT_VERSION "gpt-5.2"
#define GEMINI_VERSION "gemini-2.5-flash"
#define CLAUDE_VERSION "claude-sonnet-4-5"
#define BACKOFF 2000
#define MAX_TOKENS 1024
#define TIMEOUT_SECONDS 60L
static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string_view get_llm_name(LLM llm){
    switch (llm){
        case LLM::GPT:
            return "ChatGPT";
        case LLM::CLAUDE:
            return "Claude";
        case LLM::GEMINI:
            return "Gemini";
        default:
            throw std::logic_error("Unknown LLM");
    }
}

std::string get_first_line(const std::string& s) {
    size_t pos = s.find('\n');
    if (pos == std::string::npos) {
        return s; // No newline found, return the whole thing
    }
    return s.substr(0, pos);
}

static void validate_json_schema(const json& instance, const json& schema) {
    try {
        nlohmann::json_schema::json_validator validator;
        validator.set_root_schema(schema);   // may throw if schema invalid
        validator.validate(instance);        // throws on validation failure
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Schema validation failed: ") + e.what());
    }
}

// Parse a model-produced JSON string and validate it.
static json parse_and_validate_json_text(const std::string& text, const json& schema) {
    json instance;
    try {
        instance = json::parse(text);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Model output was not valid JSON: ") + e.what() +
                                 "\nRaw text:\n" + text);
    }
    validate_json_schema(instance, schema);
    return instance;
}

// Utility: return env var or throw
static std::string must_getenv(const char* name) {
    const char* v = std::getenv(name);
    if (!v || !*v) throw std::runtime_error(std::string("Missing environment variable: ") + name);
    return v;
}


json LLMHandler::PromptGPT(const std::string_view prompt, const json& schema) {
    std::string api_key = must_getenv("OPENAI_API_KEY");

    json body = {
        {"model", GPT_VERSION},
        {"input", json::array({ {{"role","user"},{"content", prompt}} })},
        {"store", false},
        {"text", {
            {"format", {
                {"type", "json_schema"},
                {"name", "structured_response"},
                {"strict", true},
                {"schema", schema}
            }}
        }}
    };

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("Authorization: Bearer ") + api_key).c_str());

    std::string payload = body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) throw std::runtime_error(std::string("curl error (GPT): ") + curl_easy_strerror(rc));
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("OpenAI HTTP " + std::to_string(http_code) + "\n" + response);
    }

    json j = json::parse(response);

    std::string out;
    if (j.contains("output") && j["output"].is_array()) {
        for (const auto& item : j["output"]) {
            if (!item.is_object()) continue;
            if (!item.contains("content") || !item["content"].is_array()) continue;

            for (const auto& c : item["content"]) {
                if (!c.is_object()) continue;

                // Responses API uses content blocks like {type:"output_text", text:"..."}
                if (c.value("type", "") == "output_text" && c.contains("text") && c["text"].is_string()) {
                    out = c["text"].get<std::string>();
                    break;
                }
            }
            if (!out.empty()) break;
        }
    }

    if (out.empty()) {
        throw std::runtime_error("OpenAI response had no output_text block.\nRaw:\n" + response);
    }

    return parse_and_validate_json_text(out, schema);
}

json LLMHandler::PromptGemini(const std::string_view prompt, const json& schema) {
    std::string api_key = must_getenv("GEMINI_API_KEY");
    std::string model = GEMINI_VERSION;

    json body = {
        {"contents", json::array({
            {
                {"role", "user"},
                {"parts", json::array({ {{"text", prompt}} })}
            }
        })},
        {"generationConfig", {
            {"responseMimeType", "application/json"},
            {"responseJsonSchema", schema}
        }}
    };

    std::string url =
        "https://generativelanguage.googleapis.com/v1beta/models/" +
        model + ":generateContent";

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("x-goog-api-key: ") + api_key).c_str());

    std::string payload = body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) throw std::runtime_error(std::string("curl error (Gemini): ") + curl_easy_strerror(rc));
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("Gemini HTTP " + std::to_string(http_code) + "\n" + response);
    }

    json j = json::parse(response);

    // Typical: candidates[0].content.parts[0].text is JSON text.
    std::string text;
    try {
        text = j.at("candidates").at(0).at("content").at("parts").at(0).at("text").get<std::string>();
    } catch (...) {
        throw std::runtime_error("Unexpected Gemini response format.\nRaw:\n" + response);
    }

    return parse_and_validate_json_text(text, schema);
}

json LLMHandler::PromptClaude(const std::string_view prompt, const json& schema) {
    std::string api_key = must_getenv("ANTHROPIC_API_KEY");

    std::string model = CLAUDE_VERSION;

    json body = {
        {"model", model},
        {"max_tokens", MAX_TOKENS},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"output_config", {
            {"format", {
                {"type", "json_schema"},
                {"schema", schema}
            }}
        }}
    };

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("x-api-key: ") + api_key).c_str());
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    std::string payload = body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) throw std::runtime_error(std::string("curl error (Claude): ") + curl_easy_strerror(rc));
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("Claude HTTP " + std::to_string(http_code) + "\n" + response);
    }

    json j = json::parse(response);

    // Typical: content[0].text is JSON text.
    std::string text;
    try {
        text = j.at("content").at(0).at("text").get<std::string>();
    } catch (...) {
        throw std::runtime_error("Unexpected Claude response format.\nRaw:\n" + response);
    }

    return parse_and_validate_json_text(text, schema);
}

json LLMHandler::Prompt(const std::string_view prompt, const json& schema, const LLM& llm){
    switch (llm)
    {
    case LLM::GPT:
        return PromptGPT(prompt, schema);
    case LLM::CLAUDE:
        return PromptClaude(prompt, schema);
    case LLM::GEMINI:
        return PromptGemini(prompt, schema);
    default:
        throw std::logic_error("Unknown LLM type");
    }
}

// default retries = 3

static std::unordered_set<std::string> overloaded_errors{"Claude HTTP 529", "Gemini HTTP 503", "GPT HTTP 503"};
static std::unordered_set<std::string> timeout_errors{
    "curl error (Gemini): Timeout was reached",
    "curl error (GPT): Timeout was reached",
    "curl error (Claude): Timeout was reached",
};
json LLMHandler::PromptWithRetries(const std::string_view prompt, const json& schema, const LLM& llm, unsigned int retries){
    std::cout << "Prompting " << get_llm_name(llm) << std::endl;
    unsigned int remaining_retries = retries;
    while (remaining_retries > 0){
        try {
            json rsp = Prompt(prompt, schema, llm);
            return rsp;
        } catch (std::runtime_error& e) {
            // if e is not an overloaded error, re-throw
            if (!overloaded_errors.contains(get_first_line(e.what())) && !timeout_errors.contains(get_first_line(e.what()))){
                throw e;
            } else {
                std::cout << "Caught " << std::string(get_first_line(e.what())) << std::endl;
            }
        }
        // exponential backoff: wait for 2 * (attempts) s
        int backoff = 1000 << (retries - remaining_retries);
        std::cout << "Encountered an error with " << get_llm_name(llm) << ", retrying in " << backoff << " milliseconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
        remaining_retries--;
    }
    throw std::runtime_error(std::format("Attempted to call {} {} times, but was overloaded each time", get_llm_name(llm), retries));
}

LLMHandler::LLMHandler(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
};
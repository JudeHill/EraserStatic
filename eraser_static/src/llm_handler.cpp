#include "llm_handler.h"
#define GPT_VERSION "gpt-5.2"
#define GEMINI_VERSION "gemini-2.5-pro"

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}


json LLMHandler::PromptGPT(std::string prompt){
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        std::cerr << "Set OPENAI_API_KEY in your environment.\n";
        return 1;
    }

    json body = {
        {"model", GPT_VERSION},          // or "gpt-5.2", "gpt-4o mini", etc.
        {"input", prompt},
        {"store", false}              // optional: disable storage
    };

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("Authorization: Bearer ") + api_key).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.dump().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(rc) << "\n";
        return 1;
    }
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "HTTP " << http_code << "\n" << response << "\n";
        return 1;
    }

    // The Responses API includes a convenient top-level "output_text" field.
    // (It also includes a richer "output" array of items.)
    return json::parse(response);
}

json LLMHandler::PromptGemini(std::string prompt) {
    const char* api_key = std::getenv("GEMINI_API_KEY");
    if (!api_key) {
        std::cerr << "Set GEMINI_API_KEY in your environment.\n";
        return 1;
    }

    // Model name example: "gemini-1.5-flash" or "gemini-1.5-pro"
    std::string model = GEMINI_VERSION;

    // Gemini request format
    json body = {
        {"contents", {
            {
                {"role", "user"},
                {"parts", {
                    { {"text", prompt} }
                }}
            }
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
    headers = curl_slist_append(headers,
        (std::string("x-goog-api-key: ") + api_key).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.dump().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(rc) << "\n";
        return 1;
    }

    if (http_code < 200 || http_code >= 300) {
        std::cerr << "HTTP " << http_code << "\n" << response << "\n";
        return 1;
    }

    // Parse Gemini response
    auto j = json::parse(response);

    return j;
}

json LLMHandler::PromptClaude(std::string prompt) {
    const char* api_key = std::getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        std::cerr << "Set ANTHROPIC_API_KEY in your environment.\n";
        return 1;
    }

    // Example model (pick one you have access to)
    std::string model = "claude-opus-4-6";

    // Claude Messages API request body
    json body = {
        {"model", model},
        {"max_tokens", 512},
        {"messages", json::array({
            {
                {"role", "user"},
                {"content", prompt}
            }
        })}
    };

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("x-api-key: ") + api_key).c_str());
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01"); // required :contentReference[oaicite:1]{index=1}

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.dump().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(rc) << "\n";
        return 1;
    }
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "HTTP " << http_code << "\n" << response << "\n";
        return 1;
    }

    // Parse Claude response:
    // content is an array of blocks like: [{ "type": "text", "text": "..." }]
    auto j = json::parse(response);
    return j;
}

json LLMHandler::Prompt(std::string prompt, LLM llm){
    switch (llm)
    {
    case LLM::GPT:
        return PromptGPT(prompt);
    case LLM::CLAUDE:
        return PromptClaude(prompt);
    case LLM::GEMINI:
        return PromptGemini(prompt);
    default:
        throw std::logic_error("Unknown LLM type");
    }
}

LLMHandler::LLMHandler(){};
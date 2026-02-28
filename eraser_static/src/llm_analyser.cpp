#include "llm_analyser.h"

static const std::string prompt_1 = R"(Given the following source code, the following (LLM-generated) list of shared variables and 
the following report detailing data races on certain variables please identify which are false positives. The source code will be in C, 
using pthreads. The report will be in the following format:

Found dataraces involving 1 unique variables
array.c 0 0 tids: 1 unprotected accesses
Write in file array.c at line 13, position 3

please respond in JSON schema

[{"name": str, "data_race": bool, "accesses": [{"access_type": str, line": int, "column": int, "unprotected": bool}]}]

)"

LLMAnalyser::LLMAnalyser(const Filepath& filepath) : filepath(filepath){

}

LLMAnalyser::FilterFalsePositives(){
    SharedVarInfos shared_vars = shared_var_identifier.findSharedVariables(filepath);
    json schema = {
        {"$schema", "http://json-schema.org/draft-07/schema#"},
        {"type", "array"},
        {"items",
          {
            {"type", "object"},
            {"properties",
              {
                {"name", {{"type", "string"}}},
                {"data_race", {{"type", "boolean"}}},
                {"accesses",
                  {
                    {"type", "array"},
                    {"items",
                      {
                        {"type", "object"},
                        {"properties",
                          {
                            {"access_type", {{"type", "string"}}},
                            {"line", {{"type", "integer"}, {"minimum", 0}}},
                            {"column", {{"type", "integer"}, {"minimum", 0}}},
                            {"unprotected", {{"type", "boolean"}}}
                          }
                        },
                        {"required", {"access_type", "line", "column", "unprotected"}},
                        {"additionalProperties", false}
                      }
                    }
                  }
                }
              }
            },
            {"required", {"name", "data_race", "accesses"}},
            {"additionalProperties", false}
          }
        }
      };
}
#include "llm_analyser.h"

static const std::string prompt_1 = R"(### ROLE
You are an expert C Concurrency Analyst. Your task is to perform a high-fidelity audit of reported data races in C pthreads code to distinguish between True Positives and False Positives.

### TASK DESCRIPTION
Analyze the provided Source Code, Shared Variables list, and Race Report. For each reported race, determine if the logic of the program (e.g., mutexes, semaphores, barriers, or memory offsets) effectively prevents a data race, even if the static analysis tool flagged it.

### DEFINITIONS
- A "Data Race" is defined as when one thread reads a shared variable x, and another writes to x. If these two accesses occur without a happens-before relation
between them (e.g. commonly held lock, barrier between accesses, condition variable, etc.) then this is a data race.
- For this task, you will identify locations in source code where data races **COULD** occur. For the purposes of this tool, we consider any 
case where it is theoretically possible for a data race to occur to be a data race

### CRITERIA FOR EVALUATION
When determining if a race is a False Positive, consider:
1. Mutual Exclusion: Are the accesses protected by the same pthread_mutex?
2. Logical Partitioning: Are threads accessing disjoint memory locations (e.g., different array indices `arr[tid]`)?
3. Barriers/Ordering: Is there an explicit synchronization barrier or signaling mechanism (e.g., `pthread_cond_wait`) ensuring sequential access?

### CONSTRAINTS
- In the output, ONLY include shared variables which have at least one unprotected access according to the data race report given. 
- If an access is "unprotected", this means there is a possibility for this access to be part of a data race (e.g. due to not holding a common lock / not
having a barrier between accesses. An unprotected access on a variable x implies that x has at least one data race, hence the "data_race" field
of x should be set to true if one of x's accesses is unprotected
- False positives are accesses that are not actually unprotected as defined above, but are part of the input.
- All accesses included in the Data Race Report should be included in the output, categorised as either true or false.
- There is a space for reasoning / justification. This is a complex problem, so take time to think. Put any justification in this box, but keep it 
**CONCISE**.
- Each access has an id, which is a non-negative integer. This is used to identify which accesses you are marking as unprotected. When you respond, for each access you should repeat its original ID, as well as
line, column and filename.
- The "name" field in variables should contain **ONLY** the name given to the variable in the Data Race Report and NOTHING ELSE. Anything else should be left for the reasoning section.

### OUTPUT FORMAT
You must respond strictly in the following JSON schema:
{"variables": [
  {
    "name": "string",
    "data_race": "boolean",
    "accesses": [
      {
        "access_type": "string", 
        "line": "integer", 
        "column": "integer", 
        "filename": "string",
        "id": "integer",
        "unprotected": "boolean",
        "reasoning": string
      }
    ]
  }
]}

### INPUT DATA
---
)";

RaceType convert_access_type(const std::string& access_type){
  if (access_type == "Write" || access_type == "write"){
    return RaceType::RACE_WRITE;
  }
  if (access_type == "Read" || access_type == "read"){
    return RaceType::RACE_READ;
  }
  throw std::logic_error("Unrecognised access type string");
}

template <typename T>
double calculate_fleiss_kappa_binary(const std::unordered_map<T, unsigned int>& results, int num_runs) {
    if (results.empty() || num_runs < 2) return 0.0;

    const size_t N = results.size(); // Number of items (Data Races)
    const double k = static_cast<double>(num_runs);
    
    // 1. Calculate the global proportion of TP and FP labels (p_j)
    double total_tp_votes = 0;
    for (auto const& [race, tp_count] : results) {
        total_tp_votes += tp_count;
    }

    double p_tp = total_tp_votes / (N * k);
    double p_fp = 1.0 - p_tp; // Since it's binary
    
    // Expected agreement (P_e)
    double Pe = (p_tp * p_tp) + (p_fp * p_fp);

    // 2. Calculate observed agreement for each race (P_i)
    // Formula: Pi = [1 / k(k-1)] * [ (tp^2 + fp^2) - k ]
    double sum_Pi = 0.0;
    for (auto const& [race, tp_count] : results) {
        double tp = static_cast<double>(tp_count);
        double fp = k - tp;
        
        double Pi = (std::pow(tp, 2) + std::pow(fp, 2) - k) / (k * (k - 1.0));
        sum_Pi += Pi;
    }

    // Average observed agreement (P_bar)
    double P_bar = sum_Pi / static_cast<double>(N);

    // 3. Final Kappa Calculation
    // Handle the case where Pe is 1 (all votes are the same category)
    if (std::abs(1.0 - Pe) < 1e-9) {
        return (P_bar >= 1.0 - 1e-9) ? 1.0 : 0.0;
    }

    return (P_bar - Pe) / (1.0 - Pe);
}

LLMAnalyser::LLMAnalyser(const Filepath fp) : filepath(fp){

}

JsonResults LLMAnalyser::FilterFalsePositives(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results){
    json schema = {
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"type", "object"},
      {"properties", {
        {"variables", {
          {"type", "array"},
          {"items", {
            {"type", "object"},
            {"properties", {
              {"name", {{"type", "string"}}},
              {"data_race", {{"type", "boolean"}}},
              {"accesses", {
                {"type", "array"},
                {"items", {
                  {"type", "object"},
                  {"properties", {
                    {"access_type", {{"type", "string"}}},
                    {"line", {{"type", "integer"}}},
                    {"column", {{"type", "integer"}}},
                    {"filename", {{"type", "string"}}},
                    {"id", {{"type", "integer"}}},
                    {"unprotected", {{"type", "boolean"}}},
                    {"reasoning", {{"type", "string"}}}
                  }},
                  {"required", {"access_type", "line", "column", "filename", "id", "unprotected", "reasoning"}},
                  {"additionalProperties", false}
                }}
              }}
            }},
            {"required", {"name", "data_race", "accesses"}},
            {"additionalProperties", false}
          }}
        }}
      }},
      {"required", {"variables"}},
      {"additionalProperties", false}
    };
      std::ostringstream oss;
      create_prompt(oss, filepath, prompt_1);
      oss << "\n" << "SHARED VARIABLES:" << "\n" << "---" << "\n";
      std::unordered_map<std::string, unsigned int> votes;
      for (const auto& [llm, llm_result] : shvar_results){
        for (const auto& var : llm_result.votes){
          votes[var]++;
        }
      }
      for (const auto& [var, count] : votes){
        if (count > 1){
          oss << var << "\n";
        }
      }
      oss << "\n" << "RACE REPORT:" << "\n";
      for (const auto &[var_name, data_races] : data_race_map) {
        oss << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
        for (int i = 0; i <data_races.size(); i++) {
          DataRace dr = data_races[i];
          if (dr.race_type == RaceType::RACE_READ) {
            oss << "Read";
          } else {
            oss << "Write";
          }
          oss << " in file " << dr.location.file_name << " at line " << dr.location.line
                     << ", position " << dr.location.column << " with id " << dr.id << "\n";
        }
      }
      std::string_view prompt = oss.view();
      JsonResults responses;
      std::unordered_map<LLM, std::future<std::vector<json>>> futures;
      for (const auto& llm : all_llms){
        futures[llm] = std::async(std::launch::async, [this, prompt, schema, llm]{
          return this->llm_handler.PromptWithRetries(prompt, schema, llm).at("variables").get<std::vector<json>>();
        });
      }
      for (auto& [llm, task] : futures){
        responses[llm] = task.get();
      }

      return responses;
}

void LLMAnalyser::TestFilterFalsePositives(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results){
  JsonResults results = FilterFalsePositives(data_race_map, shvar_results);
  for (const auto& [llm, result] : results){
     std::cout << get_llm_name(llm) << std::endl;
     for (const auto& var_result : result){
        std::cout << var_result.dump(4) << std::endl;
     }
  }
}

FalsePosResults LLMAnalyser::ParseResults(const JsonResults& json_results){
  FalsePosResults results;
  for (const auto& llm : all_llms){
    FalsePosResult result;
    for (const auto& var_result_json : json_results.at(llm)){
      VarResult var_result;
      var_result.has_data_race = var_result_json.at("data_race").get<bool>();
      var_result.var_name = var_result_json.at("name").get<std::string>();
      for (const json& access_json : var_result_json.at("accesses").get<std::vector<json>>()){
        unsigned int line = access_json.at("line").get<unsigned int>();
        unsigned int col = access_json.at("column").get<unsigned int>();
        std::string filename = access_json.at("filename").get<std::string>();
        std::string reasoning = access_json.at("reasoning").get<std::string>();
        std::string access_type_str = access_json.at("access_type").get<std::string>();
        RaceType access_type = convert_access_type(access_type_str);
        bool is_data_race = access_json.at("unprotected").get<bool>();
        unsigned int id = access_json.at("id").get<unsigned int>();

        DataRace data_race = {
          .var_name = var_result.var_name,
          .node = nullptr,
          .race_type = access_type,
          .location = LocationInfo{
            .file_name = filename,
            .line = line,
            .column = col,  
          },
          .id = id,
        };
        LLM_DataRace llm_data_race = {
          .data_race = data_race,
          .true_pos = is_data_race,
          .reasoning = reasoning,
        };
        if (is_data_race){
          var_result.true_pos_accesses[id] = llm_data_race;
        } else {
          var_result.false_pos_accesses[id] = llm_data_race;
        }
      } 
      result.push_back(var_result);  
    }
    results[llm] = result;
  }
  return results;
}

SummaryFalsePosResults LLMAnalyser::EvalFalsePosLLMConsistency(const DataRaceMap& data_race_map, const SharedVarResults& shvar_results, bool slow_llms, unsigned int repeats){
  std::vector<std::future<FalsePosResults>> futures;
  std::vector<FalsePosResults> results;
  for (int i=0;i<repeats;i++){
    futures.push_back(std::async(std::launch::async, [this, &data_race_map, &shvar_results](){
      return ParseResults(FilterFalsePositives(data_race_map, shvar_results));
    }));
    if (slow_llms) {
      results.push_back(futures.back().get());
      std::this_thread::sleep_for(std::chrono::seconds(LLM_API_DELAY_SECONDS));
    } 
  }
  if (!slow_llms){
    for (auto& task : futures){
      results.push_back(task.get());
    }
  }
  SummaryFalsePosResults summary_results = {
    .results = { },
    .data_race_map = data_race_map,
  };
  for (const auto& llm : all_llms){
    summary_results.results[llm] = { };
  }
  for (const auto& result : results){
    for (const auto& [llm, llm_result] : result){
      for (const auto& var_result : llm_result){
        if (var_result.has_data_race){
          summary_results.results[llm].tp_votes_variables[var_result.var_name]++;
        } else {
          summary_results.results[llm].fp_votes_variables[var_result.var_name]++;
        }
        for (const auto& [id, access] : var_result.false_pos_accesses){
          summary_results.results[llm].tp_votes_accesses[id]++;
        }
        for (const auto& [id, access] : var_result.true_pos_accesses){
          summary_results.results[llm].fp_votes_accesses[id]++;
        }
      }
    }
  }

  for(const auto& llm : all_llms){
    SummaryFalsePosResult& cur_result = summary_results.results[llm];
    cur_result.fleiss_kappa_accesses = calculate_fleiss_kappa_binary(cur_result.tp_votes_accesses, repeats);
    cur_result.fleiss_kappa_variables = calculate_fleiss_kappa_binary(cur_result.tp_votes_variables, repeats);
  }

  return summary_results;
}

void LLMAnalyser::HintsToProgrammer(){

}
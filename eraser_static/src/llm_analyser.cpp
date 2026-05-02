#include "llm_analyser.h"

static const std::string false_pos_prompt = R"(### ROLE
You are an expert C Concurrency Analyst. Your task is to perform a high-fidelity audit of reported data races in C pthreads code to distinguish between True Positives and False Positives.

### TASK DESCRIPTION
Analyze the provided Source Code, Shared Variables list, and Race Report. For each reported race, determine if the logic of the program (e.g., mutexes, semaphores, barriers, or memory offsets) effectively prevents a data race, even if the static analysis tool flagged it.

### DEFINITIONS
- A "Data Race" is defined as when one thread reads or writes to a shared variable x, and another writes to x. If these two accesses occur without a happens-before relation
between them (e.g. commonly held lock, barrier between accesses, condition variable, etc.) then this is a data race.
- For this task, you will identify locations in source code where data races **COULD** occur. For the purposes of this tool, we consider any 
case where it is theoretically possible for a data race to occur to be a data race

### CRITERIA FOR EVALUATION
When determining if a race is a False Positive, consider:
1. Mutual Exclusion: Are the accesses protected by the same pthread_mutex?
2. Logical Partitioning: Are threads accessing disjoint memory locations (e.g., different array indices `arr[tid]`)?
3. Barriers/Ordering: Is there an explicit synchronization barrier or signaling mechanism (e.g., `pthread_cond_wait`) ensuring sequential access?

### CONSTRAINTS
- In the output, ONLY include shared variables which **ARE IN THE DATA RACE REPORT** given.
- Include **ALL** variables **THAT ARE IN THE DATA RACE REPORT**. If the data race report is non-empty, your response SHOULD NOT BE EMPTY.  
- If an access is "unprotected", this means there is a possibility for this access to be part of a data race (e.g. due to not holding a common lock / not
having a barrier between accesses. An unprotected access on a variable x implies that x has at least one data race, hence the "data_race" field
of x should be set to true if one of x's accesses is unprotected
- False positives are accesses that are not actually unprotected as defined above, but are part of the input.
- All accesses included in the Data Race Report should be included in the output, categorised as either true or false.
- The ID from each access in your report should be **IDENTICAL** to the ID given in the race report below. These are used to key into a map, so it is **IMPERATIVE** you do not 
renumber them or change them in any way. Specifically, they should **STAY ZERO INDEXED**.
- You should include **ALL** accesses from the Data Race Report in your output **REGARDLESS** of your categorisation of them as protected or unprotected, 
or if you categorise a variable as having a data race or not.
- If the data race map is **EMPTY** - i.e. there are no races to report - you should return an **EMPTY LIST** in the variables field.
- There is a space for reasoning / justification. This is a complex problem, so take time to think. Put any justification in this box, but keep it 
**CONCISE**.
- Each access has an id, which is a non-negative integer. This is used to identify which accesses you are marking as unprotected. When you respond, for each access you should repeat its original ID, as well as
line, column and filename.
- The "name" field in variables should contain **ONLY** the name given to the variable in the Data Race Report and NOTHING ELSE. Anything else should be left for the reasoning section.
- "Access Type" can be STRICTLY either "read" or "write". For increments, you should report TWO accesses, one "read" and one "write". This is the
format given to you in the Data Race Report

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

static const std::string false_neg_prompt = R"(### ROLE
You are an expert C Concurrency Analyst. Your task is to perform a high-fidelity audit of C pthreads code to identify data races that were MISSED by the static analysis tool.

### TASK DESCRIPTION
Analyze the provided Source Code, Shared Variables list, and Race Report. Your goal is to find unprotected accesses that could participate in a data race, but which were NOT included in the Race Report.

---

### CRITICAL REGION RULE (MANDATORY FIRST STEP)
Before performing ANY race reasoning, you MUST partition the program into **execution regions**.

A new region begins at:
1. Calls to `pthread_create` or `pthread_join` (transition between single-threaded and multithreaded execution)
2. Every valid `pthread_barrier_wait`
3. Any other synchronization that establishes a happens-before relationship

STRICT RULES:
- Accesses in DIFFERENT regions MUST NEVER be compared.
- Two accesses separated by a valid barrier CANNOT race.
- When a barrier is reached, ALL variable locksets RESET to FULL.
- If a barrier exists but its correctness (e.g. thread count) is unclear, you MUST assume it is valid.
- You MUST only identify unprotected accesses by comparing accesses that occur within the SAME region.

FAILURE TO FOLLOW THIS RULE WILL RESULT IN INCORRECT ANALYSIS.

---

### DEFINITIONS

- A "Data Race" is defined as when one thread reads a shared variable x, and another writes to x, and BOTH accesses occur within the SAME REGION without a happens-before relation (e.g. common lock, condition variable, etc.).
- A Barrier (represented here by a call to pthread_barrier_wait) will block a thread at that barrier until ALL threads have reached that barrier. Specifically, if a thread is executing some code past the location of the call to pthread_barrier_wait, this NECESSARILY means that ALL threads MUST have passed the barrier, i.e. no thread can still be executing code BEFORE the barrier, this is IMPOSSIBLE. 
- For this task, you will identify locations in source code where data races would occur, if the program were to be executed. We consider any case where it is theoretically possible for a data race to occur WITHIN A REGION to be a data race.

- A "False Negative" is an unprotected access that is missing from the Race Report. This includes:
  1. accesses on shared variables that do not appear in the Race Report at all, and
  2. additional unprotected accesses on variables that do appear in the Race Report, where those specific accesses were omitted.

---

### UNPROTECTED ACCESS DEFINITION

An access to a shared variable is **UNPROTECTED** if it belongs to the smallest subset of accesses (WITHIN A SINGLE REGION) whose removal would eliminate all possible data races on that variable.

Formally:
- Consider ONLY accesses within the SAME REGION.
- Identify the smallest subset of accesses such that, if those accesses were removed, the remaining accesses would share a non-empty common lockset.
- Mark exactly those accesses as UNPROTECTED.

Preconditions (must be checked first, WITHIN EACH REGION):
- If all accesses are reads → NO unprotected accesses.
- If all accesses are performed by a single thread → NO unprotected accesses.

Important:
- An access is NOT unprotected merely because it could participate in a data race.
- Multiple accesses may be unprotected.
- Later accesses may also be unprotected if they independently violate the locking discipline.
- Only mark accesses whose removal is necessary to restore a consistent lockset.
- NEVER consider accesses across different regions.

---

### CRITERIA FOR EVALUATION

When determining whether an access is genuinely unprotected, consider:

1. Mutual Exclusion:
   Is the access protected by a pthread_mutex that is also held on all conflicting accesses within the SAME REGION?

2. Logical Partitioning:
   Are threads accessing disjoint memory locations (e.g., `arr[tid]`)? If so, NOT a race.

3. Barriers / Ordering:
   If accesses are separated by a barrier → THEY CANNOT RACE.
   Do NOT compare them.

4. Multithreaded Context:
   - If the region is single-threaded → NO unprotected accesses.
   - A variable must be accessed by at least TWO threads in the SAME REGION, and at least one access must be a write.

5. Synchronisation Mechanisms:
   Consider condition variables, signaling, or other ordering guarantees within the SAME REGION.

---

### HARD VALIDATION RULE (MUST FOLLOW)

For EVERY access you report as unprotected:
- You MUST be able to identify at least ONE conflicting access:
  - in the SAME REGION
  - from a DIFFERENT thread
  - where at least one access is a write
  - and where there is NO common protecting mechanism

If the ONLY conflicting accesses are across a barrier or across regions:
→ DO NOT REPORT IT.

---

### JUSTIFICATION GUIDELINES

- The reasoning for each reported access should focus primarily on **WHY the data race exists**, not the step-by-step process used to discover it.
- Clearly describe:
  - the conflicting accesses (read/write across threads),
  - the absence of mutual exclusion or other synchronization,
  - and why they can occur concurrently within the same region.
- Keep explanations **concise and direct**.
- Do NOT describe the full lockset analysis procedure or region-partitioning process unless absolutely necessary.
- Avoid meta-reasoning (e.g. “I checked…”, “the analysis shows…”). Instead, state the concurrency issue directly.

---

### CONSTRAINTS

- ONLY include shared variables which have at least one unprotected access NOT already in the Race Report.
- DO NOT repeat accesses already listed in the Race Report.
- A variable appears in output ONLY if it has at least one missing unprotected access.
- You should not include protected accesses.
- It is VERY COMMON that there are FEW or NO false negatives → returning an EMPTY LIST is valid.
- Prioritise ACCURACY over over-approximation.
- Do NOT invent accesses not present in the code.
- Distinguish carefully between:
  - variables missing entirely from the report
  - variables with additional missed accesses
- Ignore whether the existing report contains false positives.
- Each reported access MUST be capable of forming a real data race within its region.

---

### OUTPUT FORMAT

You must respond strictly in the following JSON schema:

{"variables": [
  {
    "name": "string",
    "accesses": [
      {
        "access_type": "string",
        "line": "integer",
        "column": "integer",
        "filename": "string",
        "reasoning": "string"
      }
    ]
  }
]}

---

### INPUT DATA
---
)";

static bool displayed = false;

// We need this because the var name in the Eraser system is not reliably equal to the one
// identified by the LLM: hence we need parity. We use the IDs from data races to do this
// Possible improvement: implement IDs for variables as well and key everything by these IDs instead
// of names
std::string get_var_name_key(const VarResult &var_result, const DataRaceMap &data_race_map) {
  if (!var_result.false_pos_accesses.empty()) {
    const auto &[id, access] = *var_result.false_pos_accesses.begin();
    if (!data_race_map.by_id.contains(id)) {
      throw std::logic_error(std::format("Data race map by id does not contain id {}", id));
    }
    return data_race_map.by_id.at(id)->var_name;
  }
  if (!var_result.true_pos_accesses.empty()) {
    const auto &[id, access] = *var_result.true_pos_accesses.begin();
    if (!data_race_map.by_id.contains(id)) {
      throw std::logic_error(std::format("Data race map by id does not contain id {}", id));
    }
    return data_race_map.by_id.at(id)->var_name;
  }
  throw std::logic_error(
      "Unable to identify variable name key from given VarResult - no accesses provided");
}

// A way to uniquely identify data races for LLMs. 
// We assume that a race is unique if it has the same type, variable and line number. This will not always be the case, 
// but we cannot reliably differentiate races from LLM output that are equivalent in all these things
// as LLMs cannot reliably determine the column of a specific character. 
std::string get_datarace_key(const DataRace& data_race){
  return data_race.var_name + std::to_string(data_race.location.line) + (data_race.race_type == RaceType::RACE_READ ? "Read" : "Write") + data_race.location.file_name;
}

RaceType convert_access_type(const std::string &access_type) {
  if (access_type == "Write" || access_type == "write") {
    return RaceType::RACE_WRITE;
  }
  if (access_type == "Read" || access_type == "read") {
    return RaceType::RACE_READ;
  }
  throw std::logic_error("Unrecognised access type string");
}

void dump_LLM_result(const FalsePosResult &result) {
  for (const auto &var_result : result.var_results){
    std::cout << "Var result for var " << var_result.var_name << std::endl;
    std::cout << (var_result.has_data_race ? "Yes data race" : "No data race") << std::endl;
    std::cout << "False pos votes: " << std::endl;
    for (const auto &[id, access] : var_result.false_pos_accesses) {
      std::cout << "Var name: " << access.data_race.var_name << " id: " << id
                << " True pos: " << access.true_pos;
      std::cout << " line: " << access.data_race.location.line;
      std::cout << " Access type: "
                << (access.data_race.race_type == RaceType::RACE_WRITE ? "write" : "read")
                << std::endl;
    }
  }
}

void dump_data_race_map(const DataRaceMap &data_race_map) {
  std::cout << "Data Race Map of size " << data_race_map.by_id.size() << " with by_var of size "
            << data_race_map.by_var.size() << std::endl;
  for (const auto &[id, dr] : data_race_map.by_id) {
    std::cout << "Id: " << id << " Var name: " << dr->var_name << " Line " << dr->location.line
              << " Race type " << (dr->race_type == RaceType::RACE_WRITE ? "write" : "read")
              << std::endl;
  }
}

template <typename T>
double calculate_fleiss_kappa_binary(const std::unordered_map<T, unsigned int> &results,
                                     int num_runs) {
  if (results.empty() || num_runs < 2)
    return 0.0;

  const size_t N = results.size(); // Number of items (Data Races)
  const double k = static_cast<double>(num_runs);

  // 1. Calculate the global proportion of TP and FP labels (p_j)
  double total_tp_votes = 0;
  for (auto const &[race, tp_count] : results) {
    total_tp_votes += tp_count;
  }

  double p_tp = total_tp_votes / (N * k);
  double p_fp = 1.0 - p_tp; // Since it's binary

  // Expected agreement (P_e)
  double Pe = (p_tp * p_tp) + (p_fp * p_fp);

  // 2. Calculate observed agreement for each race (P_i)
  // Formula: Pi = [1 / k(k-1)] * [ (tp^2 + fp^2) - k ]
  double sum_Pi = 0.0;
  for (auto const &[race, tp_count] : results) {
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

// Helper to calculate Jaccard for a single pair of sets
template <typename T>
double jaccard_sets(const std::unordered_set<T> &s1, const std::unordered_set<T> &s2) {
  if (s1.empty() && s2.empty())
    return 1.0; // Perfect agreement on empty sets

  size_t intersection_count = 0;
  // Iterate over the smaller set for efficiency
  const auto &[smaller, larger] = (s1.size() < s2.size()) ? std::tie(s1, s2) : std::tie(s2, s1);

  for (const auto &item : smaller) {
    if (larger.find(item) != larger.end()) {
      intersection_count++;
    }
  }

  size_t union_count = s1.size() + s2.size() - intersection_count;
  return static_cast<double>(intersection_count) / static_cast<double>(union_count);
}

template <typename T>
double jaccard_maps(const std::unordered_map<T, unsigned int> &m1, 
                    const std::unordered_map<T, unsigned int> &m2) {
    
    if (m1.empty() && m2.empty())
        return 1.0;

    size_t intersection_sum = 0;
    size_t union_sum = 0;

    // Use a single pass to find intersection and partial union
    for (const auto &[key, count1] : m1) {
        auto it = m2.find(key);
        if (it != m2.end()) {
            unsigned int count2 = it->second;
            intersection_sum += std::min(count1, count2);
            union_sum += std::max(count1, count2);
        } else {
            // Key only in m1
            union_sum += count1;
        }
    }

    // Add counts from keys only in m2
    for (const auto &[key, count2] : m2) {
        if (m1.find(key) == m1.end()) {
            union_sum += count2;
        }
    }

    if (union_sum == 0) return 0.0; 
    
    return static_cast<double>(intersection_sum) / static_cast<double>(union_sum);
}

JaccardAgreement calculate_jaccard_agreement(const std::vector<FalsePosRunInfo> &run_infos) {
  if (run_infos.size() < 2)
    return {1.0, 1.0};

  double total_var_j = 0.0;
  double total_access_j = 0.0;
  int pair_count = 0;

  // Iterate through all unique pairs (C(n, 2))
  for (size_t i = 0; i < run_infos.size(); ++i) {
    for (size_t j = i + 1; j < run_infos.size(); ++j) {

      // Calculate Jaccard for Variables (Average of TP consistency and FP consistency)
      double var_tp = jaccard_sets(run_infos[i].var_votes_tp, run_infos[j].var_votes_tp);
      double var_fp = jaccard_sets(run_infos[i].var_votes_fp, run_infos[j].var_votes_fp);
      total_var_j += (var_tp + var_fp) / 2.0;

      // Calculate Jaccard for Accesses (Average of TP consistency and FP consistency)
      double acc_tp = jaccard_sets(run_infos[i].access_votes_tp, run_infos[j].access_votes_tp);
      double acc_fp = jaccard_sets(run_infos[i].access_votes_fp, run_infos[j].access_votes_fp);
      total_access_j += (acc_tp + acc_fp) / 2.0;

      pair_count++;
    }
  }

  return {total_var_j / pair_count, total_access_j / pair_count};
}

JaccardAgreement calculate_jaccard_agreement(const std::vector<FalseNegRunInfo>& run_infos) {
  if (run_infos.size() < 2)
    return {1.0, 1.0};

  double var = 0.0;
  double acc = 0.0;
  int pair_count = 0;

  for (size_t i = 0; i < run_infos.size(); ++i) {
    for (size_t j = i + 1; j < run_infos.size(); ++j) {
      var += jaccard_sets(run_infos[i].var_votes, run_infos[j].var_votes);
      acc += jaccard_maps(run_infos[i].access_votes, run_infos[j].access_votes);
      pair_count++;
    }
  }

  return {var / pair_count, acc / pair_count};
}

LLMAnalyser::LLMAnalyser(const Filepath fp) : filepath(fp) {}

JsonResults LLMAnalyser::FilterFalsePositives(const DataRaceMap &data_race_map,
                                              const SharedVarResults &shvar_results) {
  json schema = {
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"type", "object"},
      {"properties",
       {{"variables",
         {{"type", "array"},
          {"items",
           {{"type", "object"},
            {"properties",
             {{"name", {{"type", "string"}}},
              {"data_race", {{"type", "boolean"}}},
              {"accesses",
               {{"type", "array"},
                {"items",
                 {{"type", "object"},
                  {"properties",
                   {{"access_type", {{"type", "string"}}},
                    {"line", {{"type", "integer"}}},
                    {"column", {{"type", "integer"}}},
                    {"filename", {{"type", "string"}}},
                    {"id", {{"type", "integer"}}},
                    {"unprotected", {{"type", "boolean"}}},
                    {"reasoning", {{"type", "string"}}}}},
                  {"required",
                   {"access_type", "line", "column", "filename", "id", "unprotected", "reasoning"}},
                  {"additionalProperties", false}}}}}}},
            {"required", {"name", "data_race", "accesses"}},
            {"additionalProperties", false}}}}}}},
      {"required", {"variables"}},
      {"additionalProperties", false}};
  std::ostringstream oss;
  create_prompt(oss, filepath, false_pos_prompt);
  oss << "\n" << "SHARED VARIABLES:" << "\n" << "---" << "\n";
  std::unordered_map<std::string, unsigned int> votes;
  for (const auto &[llm, llm_result] : shvar_results) {
    for (const auto &var : llm_result.votes) {
      votes[var]++;
    }
  }
  for (const auto &[var, count] : votes) {
    if (count > 1) {
      oss << var << "\n";
    }
  }
  oss << "\n" << "RACE REPORT:" << "\n";
  if (data_race_map.by_id.empty() && data_race_map.by_var.empty()) {
    oss << "No races to report. " << "\n";
  }
  for (const auto &[var_name, data_races] : data_race_map.by_var) {
    oss << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
    for (int i = 0; i < data_races.size(); i++) {
      const DataRace &dr = *data_races[i];
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
  if (!displayed) {
    // std::cout << prompt << std::endl;
  }
  displayed = true;
  JsonResults responses;
  std::unordered_map<LLM, std::future<JsonResult>> futures;
  for (const auto llm : all_llms) {
    futures[llm] = std::async(std::launch::async, [this, prompt, schema, llm] {
      json response = this->llm_handler.PromptWithRetries(prompt, schema, llm);
      return JsonResult{
        .var_json = response
          .at("variables")
          .get<std::vector<json>>(),
        .response_str = response.dump(4),
      };
    });
  }
  for (auto &[llm, task] : futures) {
    responses[llm] = task.get();
  }

  return responses;
}

void LLMAnalyser::TestFilterFalsePositives(const DataRaceMap &data_race_map,
                                           const SharedVarResults &shvar_results) {
  JsonResults results = FilterFalsePositives(data_race_map, shvar_results);
  for (const auto &[llm, result] : results) {
    std::cout << get_llm_name(llm) << std::endl;
    for (const auto &var_result : result.var_json) {
      std::cout << var_result.dump(4) << std::endl;
    }
  }
}

FalsePosResults LLMAnalyser::ParseFalsePosResults(const JsonResults &json_results) {
  FalsePosResults results;
  for (const auto llm : all_llms) {
    FalsePosResult result;
    result.response_str = json_results.at(llm).response_str;
    for (const auto &var_result_json : json_results.at(llm).var_json) {
      VarResult var_result;
      var_result.has_data_race = var_result_json.at("data_race").get<bool>();
      var_result.var_name = var_result_json.at("name").get<std::string>();
      for (const json &access_json : var_result_json.at("accesses").get<std::vector<json>>()) {
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
            .location =
                LocationInfo{
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
        if (is_data_race) {
          var_result.true_pos_accesses[id] = llm_data_race;
        } else {
          var_result.false_pos_accesses[id] = llm_data_race;
        }
      }
      result.var_results.push_back(var_result);
      
    }
    results[llm] = result;
  }
  return results;
}

JsonResults LLMAnalyser::FindFalseNegatives(const DataRaceMap &data_race_map,
                                            const SharedVarResults &shvar_results) {
  json schema = {
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"type", "object"},
      {"properties",
       {{"variables",
         {{"type", "array"},
          {"items",
           {{"type", "object"},
            {"properties",
             {{"name", {{"type", "string"}}},
              {"accesses",
               {{"type", "array"},
                {"items",
                 {{"type", "object"},
                  {"properties",
                   {{"access_type", {{"type", "string"}}},
                    {"line", {{"type", "integer"}}},
                    {"column", {{"type", "integer"}}},
                    {"filename", {{"type", "string"}}},
                    {"reasoning", {{"type", "string"}}}}},
                  {"required", {"access_type", "line", "column", "filename", "reasoning"}},
                  {"additionalProperties", false}}}}}}},
            {"required", {"name", "accesses"}},
            {"additionalProperties", false}}}}}}},
      {"required", {"variables"}},
      {"additionalProperties", false}};
  std::ostringstream oss;
  // Create prompt with line numbers to aid the LLM in locating the correct line numbers for data races
  create_prompt(oss, filepath, false_neg_prompt, true);
  oss << "\n" << "SHARED VARIABLES:" << "\n" << "---" << "\n";
  // use majority voting from LLMs to establish shared variables
  std::unordered_map<std::string, unsigned int> votes;
  for (const auto &[llm, llm_result] : shvar_results) {
    for (const auto &var : llm_result.votes) {
      votes[var]++;
    }
  }
  for (const auto &[var, count] : votes) {
    if (count > 1) {
      oss << var << "\n";
    }
  }
  oss << "\n" << "RACE REPORT:" << "\n";
  if (data_race_map.by_id.empty() && data_race_map.by_var.empty()) {
    oss << "No races to report. " << "\n";
  }
  for (const auto &[var_name, data_races] : data_race_map.by_var) {
    oss << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
    for (int i = 0; i < data_races.size(); i++) {
      const DataRace &dr = *data_races[i];
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
  std::unordered_map<LLM, std::future<JsonResult>> futures;
  for (const auto llm : all_llms) {
    futures[llm] = std::async(std::launch::async, [this, prompt, schema, llm] {
      json response = this->llm_handler.PromptWithRetries(prompt, schema, llm);
      return JsonResult{
        .var_json = response
          .at("variables")
          .get<std::vector<json>>(),
        .response_str = response.dump(4),
      };
    });
  }
  for (auto &[llm, task] : futures) {
    responses[llm] = task.get();
  }


  return responses;
}

FalseNegResults LLMAnalyser::ParseFalseNegResults(const JsonResults &json_results) {
  FalseNegResults results;
  for (const auto llm : all_llms) {
    FalseNegResult result;
    for (const auto &var_result_json : json_results.at(llm).var_json) {
      FalseNegVarResult var_result;
      var_result.var_name = var_result_json.at("name").get<std::string>();
      for (const json &access_json : var_result_json.at("accesses").get<std::vector<json>>()) {
        unsigned int line = access_json.at("line").get<unsigned int>();
        unsigned int col = access_json.at("column").get<unsigned int>();
        std::string filename = access_json.at("filename").get<std::string>();
        std::string reasoning = access_json.at("reasoning").get<std::string>();
        std::string access_type_str = access_json.at("access_type").get<std::string>();
        RaceType access_type = convert_access_type(access_type_str);

        DataRace data_race = {
            .var_name = var_result.var_name,
            .node = nullptr,
            .race_type = access_type,
            .location =
                LocationInfo{
                    .file_name = filename,
                    .line = line,
                    .column = col,
                },
            .id = next_race_id,
        };
        LLM_DataRace llm_data_race = {
            .data_race = data_race,
            .true_pos = true,
            .reasoning = reasoning,
        };
        var_result.unprotected_accesses.push_back(llm_data_race);
      }
      result.push_back(var_result);
    }
    results[llm] = result;
  }
  return results;
}

SummaryFalsePosResults
LLMAnalyser::EvalFalsePosLLMConsistency(const DataRaceMap &data_race_map,
                                        const SharedVarResults &shvar_results, bool slow_llms,
                                        unsigned int repeats) {
  std::vector<std::future<FalsePosResults>> futures;
  std::vector<FalsePosResults> results;
  for (int i = 0; i < repeats; i++) {
    futures.push_back(std::async(std::launch::async, [this, &data_race_map, &shvar_results]() {
      return ParseFalsePosResults(FilterFalsePositives(data_race_map, shvar_results));
    }));
    if (slow_llms) {
      results.push_back(futures.back().get());
      std::this_thread::sleep_for(std::chrono::seconds(LLM_API_DELAY_SECONDS));
    }
  }
  if (!slow_llms) {
    for (auto &task : futures) {
      results.push_back(task.get());
    }
  }
  SummaryFalsePosResults summary_results = {
      .results = {},
      .data_race_map = data_race_map,
  };
  FalsePosRunInfos run_infos;
  for (const auto llm : all_llms) {
    summary_results.results[llm] = {};
  }
  for (const auto &result : results) {
    for (const auto &[llm, llm_result] : result) {
      FalsePosRunInfo run_info;
      for (const auto &var_result : llm_result.var_results) {
        std::string var_name_key;
        try {
          var_name_key = get_var_name_key(var_result, data_race_map);
        } catch (const std::logic_error &e) {
          // dump_LLM_result(llm_result);
          // dump_data_race_map(data_race_map);
          std::cout << e.what() << std::endl;
          std::cout << std::format("Error getting var name key: {}", var_result.var_name)
                    << std::endl;
          throw e;
        }

        if (var_result.has_data_race) {
          summary_results.results[llm].tp_votes_variables[var_name_key]++;
          run_info.var_votes_tp.insert(var_name_key);
        } else {
          summary_results.results[llm].fp_votes_variables[var_name_key]++;
          run_info.var_votes_fp.insert(var_name_key);
        }
        for (const auto &[id, access] : var_result.false_pos_accesses) {
          summary_results.results[llm].fp_votes_accesses[id]++;
          run_info.access_votes_fp.insert(id);
        }
        for (const auto &[id, access] : var_result.true_pos_accesses) {
          summary_results.results[llm].tp_votes_accesses[id]++;
          run_info.access_votes_tp.insert(id);
        }
      }
      // Check consistency
      if (!run_infos[llm].empty()) {
        const FalsePosRunInfo &last = run_infos[llm].back();
        size_t num_accesses = last.access_votes_fp.size() + last.access_votes_tp.size();
        size_t num_vars = last.var_votes_fp.size() + last.var_votes_tp.size();
        size_t new_num_accesses = run_info.access_votes_fp.size() + run_info.access_votes_tp.size();
        size_t new_num_vars = run_info.var_votes_fp.size() + run_info.var_votes_tp.size();
        bool inconsistency = false;
        if (num_accesses != new_num_accesses) {
          std::cout << "Inconsistent number of access votes: " << num_accesses
                    << " votes in last run, but " << new_num_accesses << " votes now" << std::endl;
          inconsistency = true;
        }
        if (num_vars != new_num_vars) {
          std::cout << "Inconsistent number of var votes: " << num_vars
                    << " votes in last run, but " << new_num_vars << " votes now" << std::endl;
          inconsistency = true;
        }
        if (inconsistency) {
          std::cout << "Dumping response" << std::endl;
          std::cout << llm_result.response_str << std::endl;
        }
      }
      run_infos[llm].push_back(run_info);
      std::cout << "Added run number " << run_infos[llm].size() << " to " << get_llm_name(llm)
                << std::endl;
    }
  }

  for (const auto llm : all_llms) {
    SummaryFalsePosResult &cur_result = summary_results.results[llm];
    cur_result.jaccard_agreement = calculate_jaccard_agreement(run_infos[llm]);
  }

  return summary_results;
}

SummaryFalseNegResults
LLMAnalyser::EvalFalseNegLLMConsistency(const DataRaceMap &data_race_map, const SharedVarResults &shvar_results, bool slow_llms, unsigned int repeats) {
  std::vector<std::future<FalseNegResults>> futures;
  std::vector<FalseNegResults> results;
  for (int i = 0; i < repeats; i++) {
    futures.push_back(std::async(std::launch::async, [this, &data_race_map, &shvar_results]() {
      return ParseFalseNegResults(FindFalseNegatives(data_race_map, shvar_results));
    }));
    if (slow_llms) {
      results.push_back(futures.back().get());
      std::this_thread::sleep_for(std::chrono::seconds(LLM_API_DELAY_SECONDS));
    }
  }
  if (!slow_llms) {
    for (auto &task : futures) {
      results.push_back(task.get());
    }
  }
  SummaryFalseNegResults summary_results = {
      .results = {},
      .data_race_map = data_race_map,
  };
  FalseNegRunInfos run_infos;
  for (const auto llm : all_llms) {
    summary_results.results[llm] = {};
  }
  for (const auto &result : results) {
    for (const auto &[llm, llm_result] : result) {
      FalseNegRunInfo run_info;
      for (const auto &var_result : llm_result) {
        FalseNegVariable& var = summary_results.results[llm].variables[var_result.var_name];
        var.votes++;
        run_info.var_votes.insert(var_result.var_name);
        for (const auto& access : var_result.unprotected_accesses){
          std::string datarace_key = get_datarace_key(access.data_race);
          if (!var.accesses.contains(datarace_key)){
            var.accesses[datarace_key] = FalseNegAccess{
              .access = access,
              .votes = 0,
            };
          }
          std::cout << "Voting for key " << datarace_key << " With llm " << get_llm_name(llm) << std::endl;
          var.accesses[datarace_key].votes++;
          run_info.access_votes[datarace_key]++;
        }
      }
     
      run_infos[llm].push_back(run_info);
      std::cout << "Added run number " << run_infos[llm].size() << " to " << get_llm_name(llm)
                << std::endl;
    }
  }

  for (const auto llm : all_llms) {
    SummaryFalseNegResult &cur_result = summary_results.results[llm];
    cur_result.jaccard_agreement = calculate_jaccard_agreement(run_infos[llm]);
  }
  return summary_results;                         
}

void LLMAnalyser::HintsToProgrammer() {}
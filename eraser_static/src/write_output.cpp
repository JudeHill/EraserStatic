#include "write_output.h"

void write_data_races(std::ofstream& out_stream, DataRaceMap data_race_map, bool write_all_races){
    out_stream << std::format("Found dataraces involving {} unique variables", data_race_map.size())
               << std::endl;
    
    for (const auto &[var_name, data_races] : data_race_map) {
      int num_races_to_write = write_all_races ? data_races.size() : std::min<std::size_t>(ACCESSES_PER_VAR, data_races.size());
      out_stream << var_name << ": " << data_races.size() << " unprotected accesses" << "\n";
      for (int i = 0; i < num_races_to_write; i++) {
        DataRace dr = data_races[i];
        if (dr.race_type == RaceType::RACE_READ) {
          out_stream << "Read";
        } else {
          out_stream << "Write";
        }
        out_stream << " in file " << dr.location.file_name << " at line " << dr.location.line
                   << ", position " << dr.location.column << "\n";
      }
      if (data_races.size() > num_races_to_write) {
        out_stream << "..." << "\n";
      }
    }
}

void write_shared_variables(std::ofstream& out_stream, SharedVarResults shvar_results){
    out_stream << "LLM analysis of shared variables:" << "\n";
    for (const auto &[llm, result] : shvar_results) {
        out_stream << get_llm_name(llm) << "\n";
        out_stream << "TP: " << result.true_pos << "   FP: " << result.false_pos
                    << "   FN: " << result.false_neg << "\n";
    }
}

void write_false_positives(std::ofstream& out_stream, FalsePosResults false_pos_results){
    out_stream << "LLM analysis of false positives of data races: " << "\n";
    for (const auto& llm : all_llms){
        out_stream << get_llm_name(llm) << "\n";
        for (const VarResult& var_result : false_pos_results[llm]){
        out_stream << "Variable " << var_result.var_name << "\n";
        out_stream << (var_result.has_data_race ? "Has a data race" : "No data race") << "\n";
        out_stream << var_result.true_pos_accesses.size() << " true unprotected accesses, ";
        out_stream << var_result.false_pos_accesses.size() << " false positives" << "\n";
        out_stream << "False positives: " << "\n";
        for (const auto& [_, llm_data_race] : var_result.false_pos_accesses){
            out_stream << (llm_data_race.data_race.race_type == RaceType::RACE_WRITE ? "Write " : "Read ");
            out_stream << "in file " << llm_data_race.data_race.location.file_name << ", ";
            out_stream << "on line " << llm_data_race.data_race.location.line << " ";
            out_stream << "at position " << llm_data_race.data_race.location.column << "\n";
            out_stream << "Reasoning: " << llm_data_race.reasoning << "\n";
        } 
        out_stream << "True positives: " << "\n";
        for (const auto& [_, llm_data_race] : var_result.true_pos_accesses){
            out_stream << (llm_data_race.data_race.race_type == RaceType::RACE_WRITE ? "Write " : "Read ");
            out_stream << "in file " << llm_data_race.data_race.location.file_name << ", ";
            out_stream << "on line " << llm_data_race.data_race.location.line << " ";
            out_stream << "at position " << llm_data_race.data_race.location.column << "\n";
            out_stream << "Reasoning: " << llm_data_race.reasoning << "\n";
        }  
        }
    }
}
void write_fp_eval(std::ostream& out_stream, SummaryFalsePosResults results){
    out_stream << "LLM analysis of false positives of data races: " << "\n";
    for (const auto& [var, data_races] : results.data_race_map){
        out_stream << "Eraser tool found " << data_races.size() << " unprotected accesses on variable ";
        out_stream << var << "\n";
        out_stream << "LLM votes that this variable actually has a race: ";
        for (const auto& llm : all_llms){
            out_stream << get_llm_name(llm) << ": " << results.results[llm].tp_votes_variables[var] << ", ";
        }
        out_stream << "\n";
        for (const auto& data_race : data_races){
            out_stream << (data_race.race_type == RaceType::RACE_WRITE ? "Write " : "Read ");
            out_stream << " on line " << data_race.location.line << ", at position " << data_race.location.column;
            out_stream << ", with id " << data_race.id << "\n";
            out_stream << "LLM votes that this access is actually unprotected: ";
            for (const auto& llm : all_llms){
                out_stream << get_llm_name(llm) << ": " << results.results[llm].tp_votes_accesses[data_race.id] << ", ";
            }
            out_stream << "\n";
        }
    }
    
}

void write_output(const Filepath& filepath, const Results& results, bool write_all_races) {
    auto out_stream = std::ofstream(filepath);
    if (!out_stream) {
        throw std::system_error(errno, std::generic_category(),
                                "failed to open output file: " + filepath);
    }
    write_data_races(out_stream, results.data_race_map, write_all_races);
    out_stream << "\n";
    write_shared_variables(out_stream, results.shvar_results);
    out_stream << "\n";
    write_false_positives(out_stream, results.false_pos_results);
}

void write_fp_eval_output(const Filepath& filepath, const EvalLLMResults& results, bool write_all_races){
    auto out_stream = std::ofstream(filepath);
    if (!out_stream) {
        throw std::system_error(errno, std::generic_category(),
                                "failed to open output file: " + filepath);
    }
    write_fp_eval(out_stream, results.false_pos_results);
}